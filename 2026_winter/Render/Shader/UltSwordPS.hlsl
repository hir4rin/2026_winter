// 必殺技の長い刀(red_katana_ult)を描くピクセルシェーダー(SwordLook が使う)
// 刀身の峰側(黒): つやのある黒い鋼に、切っ先へ流れる赤い血管
// 刀身の刃側(赤): 流れる血の光と、根元から先へ走る脈。刃先は白く熱く光る
// 柄・鍔: ふつうの光
// この刀は UV が使えないので、ワールドの位置を刀のモデルの空間に戻して、根元〜切っ先・峰〜刃先を求める
// 頂点シェーダーは MV1VertexShader.hlsl。BUMPMAP のときは VS_OUTPUT の並びが変わるので、同じマクロを付けてコンパイルする

Texture2D g_BaseTex : register(t0);
SamplerState g_BaseSampler : register(s0);

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

cbuffer cbD3D11_CONST_BUFFER_COMMON : register(b0)
{
	DX_D3D11_CONST_BUFFER_COMMON g_Common;
};

// ステージの明かり(b4)
#include "StageLight.hlsli"

// SwordLook::UltConstant と同じ並び
cbuffer UltSwordBuffer : register(b12)
{
	float4 g_CameraPos;		// xyz:カメラの位置 w:時間(フレーム)
	float4 g_KeyDir;		// xyz:光の来る向き w:斬りの光(0～1)
	float4 g_KeyColor;		// rgb:平行光 w:部品(0:柄・鍔 1:峰側 2:刃側)
	float4 g_SkyColor;		// rgb:上からの環境光 w:映り込みの強さ
	float4 g_GroundColor;	// rgb:下からの環境光 w:ふちの光の強さ
	float4 g_EnvSky;		// rgb:映り込みの空
	float4 g_EnvHorizon;	// rgb:映り込みの地平線の帯
	float4 g_EnvStrip;		// rgb:映り込みの縦の帯
	float4 g_Core;			// rgb:黒い鋼の反射の強さ w:ざらつき
	float4 g_BloodDeep;		// rgb:血の暗い色 w:流れる速さ
	float4 g_BloodBright;	// rgb:血の明るい色 w:脈の間隔(フレーム)
	float4 g_EdgeHot;		// rgb:刃先の熱い色 w:血管の強さ
	float4 g_Vein;			// rgb:血管の色 w:斬るときの強さの足し分
	float4 g_Shape;			// x:切っ先の z y:根元の z z:黒と赤の境目の y w:刃先の y(刀のモデルの空間)
	float4 g_Shape2;		// x:峰の y
	float4 g_ToLocal[4];	// ワールド → 刀のモデルの空間(行ベクトル × 行列)
};

static const float PI = 3.14159265f;

float Hash(float2 p)
{
	return frac(sin(dot(p, float2(127.1f, 311.7f))) * 43758.5453f);
}

float Noise(float2 p)
{
	float2 i = floor(p);
	float2 f = frac(p);
	float2 u = f * f * (3.0f - 2.0f * f);
	return lerp(lerp(Hash(i), Hash(i + float2(1, 0)), u.x), lerp(Hash(i + float2(0, 1)), Hash(i + float2(1, 1)), u.x), u.y);
}

float Fbm(float2 p)
{
	return Noise(p) * 0.55f + Noise(p * 2.1f + 3.1f) * 0.3f + Noise(p * 4.3f + 7.7f) * 0.15f;
}

float3 Env(float3 dir)
{
	float up = dir.y;
	float3 color = lerp(float3(0.01f, 0.005f, 0.008f), g_EnvSky.rgb, smoothstep(-0.35f, 0.6f, up));
	float horizon = (up - 0.03f) / 0.07f;
	color += g_EnvHorizon.rgb * exp(-horizon * horizon);
	float az = atan2(dir.z, dir.x);
	color += g_EnvStrip.rgb * pow(saturate(cos(az * 2.0f + 0.6f)), 30.0f) * smoothstep(0.05f, 0.45f, up);
	return color;
}

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
	float4 base = g_BaseTex.Sample(g_BaseSampler, input.TexCoords0_1.xy);
	float3 pos = input.WorldPosition;
	float3 normal = normalize(input.WorldNormal);
	float3 toCamera = normalize(g_CameraPos.xyz - pos);
	float3 toKey = normalize(g_KeyDir.xyz);
	float nv = saturate(dot(normal, toCamera));
	float fresnelBase = pow(1.0f - nv, 5.0f);
	float3 reflectDir = reflect(-toCamera, normal);
	float3 ambient = lerp(g_GroundColor.rgb, g_SkyColor.rgb, normal.y * 0.5f + 0.5f);
	float3 stageLight = CalcStageLights(pos, normal, IsIndoor(pos));
	float time = g_CameraPos.w;
	float slash = g_KeyDir.w;
	float part = g_KeyColor.w;

	// 刀のモデルの空間での位置(根元0～切っ先1)
	float3 local = pos.x * g_ToLocal[0].xyz + pos.y * g_ToLocal[1].xyz + pos.z * g_ToLocal[2].xyz + g_ToLocal[3].xyz;
	float along = saturate((g_Shape.y - local.z) / (g_Shape.y - g_Shape.x));

	// 脈: 根元から切っ先へ走る光と、心臓の鼓動のような明るさの揺れ
	float period = max(g_BloodBright.w * (1.0f - slash * 0.5f), 1.0f);
	float phase = frac(time / period);
	float wave = exp(-pow((phase * 1.5f - 0.25f - along) / 0.07f, 2.0f));
	float beat = exp(-pow(phase / 0.06f, 2.0f)) + 0.6f * exp(-pow((phase - 0.2f) / 0.06f, 2.0f)) + exp(-pow((phase - 1.0f) / 0.06f, 2.0f));
	float flow = time * g_BloodDeep.w * (1.0f + slash * 2.0f);
	float boost = 1.0f + slash * g_Vein.w;

	float3 color;
	if (part > 1.5f)
	{
		// 刃側: 流れる血の光
		float edge = saturate((local.y - g_Shape.z) / (g_Shape.w - g_Shape.z));	// 境目0～刃先1
		float n = Fbm(float2(along * 26.0f - flow, edge * 2.5f + local.x * 40.0f));
		float streak = smoothstep(0.5f, 0.85f, n);
		color = lerp(g_BloodDeep.rgb, g_BloodBright.rgb, saturate(0.3f + edge * 0.5f + streak * 0.6f)) * (0.85f + 0.25f * beat);	// 遠くからでも赤く見えるよう底上げ
		color += g_BloodBright.rgb * wave * 0.9f;
		color += g_EdgeHot.rgb * smoothstep(0.78f - slash * 0.25f, 1.0f, edge) * (0.55f + 0.45f * beat + slash * 0.6f);	// 斬っている間は熱い所が広がる
		// ぬれたようなつや
		color += Specular(normal, toKey, toCamera, float3(0.06f, 0.06f, 0.06f), 0.18f) * g_KeyColor.rgb * 0.7f;
		color += Env(reflectDir) * fresnelBase * 0.35f * g_SkyColor.w;
		color *= boost;
	}
	else if (part > 0.5f)
	{
		// 峰側: つやのある黒い鋼と赤い血管
		float across = saturate((local.y - g_Shape2.x) / (g_Shape.z - g_Shape2.x));	// 峰0～境目1
		float3 f0 = g_Core.rgb;
		float3 fresnel = f0 + (1.0f - f0) * fresnelBase;
		color = Env(reflectDir) * fresnel * g_SkyColor.w;
		color += Specular(normal, toKey, toCamera, f0, max(g_Core.w, 0.04f)) * g_KeyColor.rgb;
		color += stageLight * f0;
		float v = Fbm(float2(along * 38.0f - flow * 0.6f, across * 5.0f + local.x * 30.0f));
		float vein = 1.0f - smoothstep(0.0f, 0.07f, abs(v - 0.5f));	// 細すぎると遠くで粒になってちらつく
		vein *= smoothstep(0.15f, 0.9f, across) * (0.35f + 0.65f * along);
		color += g_Vein.rgb * vein * g_EdgeHot.w * (0.5f + 0.5f * beat + wave) * boost;
		color += g_BloodBright.rgb * pow(across, 8.0f) * 0.45f * boost;	// 赤い側からのにじみ
		color += g_BloodBright.rgb * fresnelBase * g_GroundColor.w;			// ふちの赤い光
	}
	else
	{
		// 柄・鍔
		float3 albedo = base.rgb * input.Diffuse.rgb;
		float nl = saturate(dot(normal, toKey));
		color = albedo * (nl * g_KeyColor.rgb + ambient);
		color += Specular(normal, toKey, toCamera, float3(0.06f, 0.06f, 0.06f), 0.3f) * g_KeyColor.rgb;
		color += albedo * stageLight;
		color += g_BloodBright.rgb * fresnelBase * g_GroundColor.w * 0.3f;
	}

	color = lerp(g_Common.Fog.Color.rgb, color, saturate(input.Fog.x * input.Fog.y));
	return float4(color, 1.0f);
}
