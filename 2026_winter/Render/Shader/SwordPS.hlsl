// プレイヤーの刀を描くピクセルシェーダー(SwordLook が使う)
// 刀身: 鏡のような鋼(まわりの景色の映り込み)・刃文・刃先の光・根元から先へ走る光。斬っている間は強く光る
// 柄・鍔など: マスク画像(R:金属 G:遮蔽 A:なめらかさ)で金属と布を分ける
// 頂点シェーダーは MV1VertexShader.hlsl。BUMPMAP のときは VS_OUTPUT の並びが変わるので、同じマクロを付けてコンパイルする

Texture2D g_BaseTex : register(t0);		// 色
SamplerState g_BaseSampler : register(s0);
Texture2D g_NormalTex : register(t1);		// 法線マップ
SamplerState g_NormalSampler : register(s1);
Texture2D g_MaskTex : register(t2);		// マスク画像
SamplerState g_MaskSampler : register(s2);

// MV1VertexShader.hlsl の VS_OUTPUT と同じ並び
struct PS_INPUT
{
	float4 Diffuse         : COLOR0;
	float4 Specular        : COLOR1;
	float4 TexCoords0_1    : TEXCOORD0;
	float3 VPosition       : POSITION0;
	float3 VNormal         : NORMAL0;
#ifdef BUMPMAP
	float3 VTan            : TANGENT;
	float3 VBin            : BINORMAL;
#endif
	float2 Fog             : TEXCOORD5;
	float4 Position        : SV_POSITION;
	float3 WorldPosition   : Position1;
	float3 WorldNormal     : Normal1;
};

#include "DataType.h"
#include "DxShader_Common_D3D11.h"

// フォグの色(DxLib の共通の値)
cbuffer cbD3D11_CONST_BUFFER_COMMON : register(b0)
{
	DX_D3D11_CONST_BUFFER_COMMON g_Common;
};

// ステージの明かり(b4)
#include "StageLight.hlsli"

// SwordLook::SwordConstant と同じ並び
cbuffer SwordBuffer : register(b11)
{
	float4 g_CameraPos;		// xyz:カメラの位置 w:時間(フレーム)
	float4 g_KeyDir;		// xyz:光の来る向き w:斬りの光(0～1)
	float4 g_KeyColor;		// rgb:平行光 w:走る光の間隔(フレーム)
	float4 g_SkyColor;		// rgb:上からの環境光 w:映り込みの強さ
	float4 g_GroundColor;	// rgb:下からの環境光 w:ふちの光の強さ
	float4 g_EnvSky;		// rgb:映り込みの空
	float4 g_EnvGround;		// rgb:映り込みの地面
	float4 g_EnvHorizon;	// rgb:映り込みの地平線の光の帯
	float4 g_EnvStrip;		// rgb:映り込みの縦の光の帯
	float4 g_Steel;			// rgb:地鉄の色 w:地鉄のざらつき
	float4 g_Hamon;			// rgb:刃文の色 w:刃文の位置(刃先0～峰1)
	float4 g_Edge;			// rgb:刃先の光の色 w:ふだんの強さ
	float4 g_Sweep;			// rgb:走る光の色 w:幅
	float4 g_SwordParam;			// x:斬るときの刃先の光の強さ y:法線マップの緑の向き z:刃先の光の幅 w:斬るときの映り込みの強さ
	float4 g_Blade;			// x:刃先の U y:刃先から峰までの U の幅 z:根元の V w:切っ先の V
};

static const float PI = 3.14159265f;

// 法線マップの向きを、近くのピクセルとの差(ddx/ddy)からワールドに直す(CharacterLookPS と同じ)
float3 ApplyNormalMap(float3 normal, float3 pos, float2 uv, float3 mapNormal)
{
	float3 dp1 = ddx(pos);
	float3 dp2 = ddy(pos);
	float2 duv1 = ddx(uv);
	float2 duv2 = ddy(uv);
	float3 dp2perp = cross(dp2, normal);
	float3 dp1perp = cross(normal, dp1);
	float3 tangent = dp2perp * duv1.x + dp1perp * duv2.x;
	float3 binormal = dp2perp * duv1.y + dp1perp * duv2.y;
	float invMax = rsqrt(max(max(dot(tangent, tangent), dot(binormal, binormal)), 1e-20f));
	return normalize(tangent * invMax * mapNormal.x + binormal * invMax * mapNormal.y + normal * mapNormal.z);
}

// 映り込む景色(本物の景色ではなく、空・地平線・縦の光の帯の作り物)
float3 Env(float3 dir)
{
	float up = dir.y;
	float3 color = lerp(g_EnvGround.rgb, g_EnvSky.rgb, smoothstep(-0.35f, 0.6f, up));
	float horizon = (up - 0.03f) / 0.07f;
	color += g_EnvHorizon.rgb * exp(-horizon * horizon);
	float az = atan2(dir.z, dir.x);
	float strip = pow(saturate(cos(az * 2.0f + 0.6f)), 30.0f) * smoothstep(0.05f, 0.45f, up);
	color += g_EnvStrip.rgb * strip;
	return color;
}

// GGX の光沢(光の色は呼ぶ側で掛ける)
float3 Specular(float3 normal, float3 toLight, float3 toCamera, float3 f0, float rough)
{
	float nl = saturate(dot(normal, toLight));
	float nv = saturate(dot(normal, toCamera)) + 1e-4f;
	float3 halfVec = normalize(toLight + toCamera);
	float nh = saturate(dot(normal, halfVec));
	float a2 = rough * rough;
	float denom = nh * nh * (a2 - 1.0f) + 1.0f;
	float distribution = a2 / (PI * denom * denom);
	float k = rough * 0.5f;
	float visibility = 0.25f / ((nl * (1.0f - k) + k) * (nv * (1.0f - k) + k));
	float fc = 1.0f - saturate(dot(toCamera, halfVec));
	float fc2 = fc * fc;
	float3 fresnel = f0 + (1.0f - f0) * (fc2 * fc2 * fc);
	return distribution * visibility * fresnel * nl;
}

float4 main(PS_INPUT input) : SV_TARGET
{
	float2 uv = input.TexCoords0_1.xy;
	float4 base = g_BaseTex.Sample(g_BaseSampler, uv);
	float4 mask = g_MaskTex.Sample(g_MaskSampler, uv);
	float3 pos = input.WorldPosition;
	float3 normal = normalize(input.WorldNormal);
	float3 mapNormal = g_NormalTex.Sample(g_NormalSampler, uv).xyz * 2.0f - 1.0f;
	mapNormal.y *= g_SwordParam.y;
	normal = ApplyNormalMap(normal, pos, uv, normalize(mapNormal));

	float3 toCamera = normalize(g_CameraPos.xyz - pos);
	float3 toKey = normalize(g_KeyDir.xyz);
	float nv = saturate(dot(normal, toCamera));
	float3 reflectDir = reflect(-toCamera, normal);
	float fresnelBase = pow(1.0f - nv, 5.0f);
	float slash = g_KeyDir.w;
	float3 ambient = lerp(g_GroundColor.rgb, g_SkyColor.rgb, normal.y * 0.5f + 0.5f);
	float3 stageLight = CalcStageLights(pos, normal, IsIndoor(pos));

	// 刀身か(UV の左の帯で金属の所)
	float edgeDist = abs(uv.x - g_Blade.x) / g_Blade.y;	// 刃先0～峰1
	bool isBlade = edgeDist < 1.05f && uv.y < g_Blade.z + 0.01f && mask.r > 0.5f;

	float3 color;
	if (isBlade)
	{
		float along = saturate((g_Blade.z - uv.y) / (g_Blade.z - g_Blade.w));	// 根元0～切っ先1

		// 刃文: 刃先の側を白くにごらせる。境目は波打つ
		float line0 = g_Hamon.w + 0.06f * sin(along * 38.0f) + 0.035f * sin(along * 97.0f + 1.7f);
		float hamon = 1.0f - smoothstep(line0 - 0.05f, line0 + 0.05f, edgeDist);
		float nioi = exp(-pow((edgeDist - line0) / 0.035f, 2.0f));	// 境目の細い光

		float3 f0 = lerp(g_Steel.rgb, g_Hamon.rgb, hamon * 0.6f);
		float rough = lerp(g_Steel.w, 0.32f, hamon);

		// 映り込み(ざらついた所はぼかしの代わりに面の向きの景色を混ぜる)
		float3 env = lerp(Env(reflectDir), Env(normal) * 0.6f, saturate(rough * 2.0f));
		float envStrength = g_SkyColor.w * (1.0f + slash * g_SwordParam.w);
		float3 fresnel = f0 + (1.0f - f0) * fresnelBase;
		color = env * fresnel * envStrength;
		color += g_Hamon.rgb * hamon * 0.12f * (ambient + g_KeyColor.rgb * saturate(dot(normal, toKey)));	// 刃文のにごり
		color += Specular(normal, toKey, toCamera, f0, max(rough, 0.04f)) * g_KeyColor.rgb;
		color += stageLight * (f0 * 0.6f + hamon * 0.15f);
		color += g_Hamon.rgb * nioi * 0.25f * (0.4f + slash);

		// 刃先の光
		float edge = 1.0f - smoothstep(0.0f, g_SwordParam.z, edgeDist);
		float flicker = 0.85f + 0.15f * sin(g_CameraPos.w * 0.15f + along * 9.0f);
		color += g_Edge.rgb * edge * (g_Edge.w * flicker + slash * g_SwordParam.x);
		color += g_Edge.rgb * (hamon * 0.5f + nioi * 0.6f) * slash;	// 斬っている間は刃文の側も光らせる(刃先の線は細くて見えにくいため)

		// 根元から先へ走る光(斬っている間は速く)
		float period = max(g_KeyColor.w * (1.0f - slash * 0.75f), 1.0f);
		float sweepPos = frac(g_CameraPos.w / period) * 1.6f - 0.3f;
		float sweep = exp(-pow((along - sweepPos) / g_Sweep.w, 2.0f));
		color += g_Sweep.rgb * sweep * (0.35f + 0.65f * hamon + fresnelBase) * (1.0f + slash);

		// ふちの光(暗い所でも刀の形が分かるように)
		color += g_Edge.rgb * fresnelBase * g_GroundColor.w;
	}
	else
	{
		// 柄・鍔など
		float metal = mask.r;
		float smooth = mask.a;
		float ao = lerp(0.6f, 1.0f, mask.g);
		float3 albedo = base.rgb;
		float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metal);
		float rough = max((1.0f - smooth) * (1.0f - smooth), 0.04f);
		float3 diffuseColor = albedo * (1.0f - metal * 0.35f);	// 金属も暗くなりすぎないよう少し拡散を残す
		float nl = saturate(dot(normal, toKey));
		color = diffuseColor * (nl * g_KeyColor.rgb + ambient * ao);
		color += Specular(normal, toKey, toCamera, f0, rough) * g_KeyColor.rgb;
		float3 env = lerp(Env(reflectDir), Env(normal) * 0.6f, saturate(rough * 2.0f));
		color += env * (f0 + (1.0f - f0) * fresnelBase) * metal * g_SkyColor.w * 0.35f * ao;	// 強いと鍔の金色が銀に見える
		color += albedo * stageLight * ao;
		color += g_Edge.rgb * fresnelBase * g_GroundColor.w * 0.3f;
	}

	// フォグ(頂点シェーダーが Fog.x に計算済み。フォグが無いときは 1)
	color = lerp(g_Common.Fog.Color.rgb, color, saturate(input.Fog.x * input.Fog.y));
	return float4(color, 1.0f);
}
