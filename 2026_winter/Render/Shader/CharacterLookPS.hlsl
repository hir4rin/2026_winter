// キャラを「法線マップ・遮蔽・光沢つき」で描くピクセルシェーダー(CharacterLook が使う)
// Mv1LikeUnity(Unity の見た目の移植)は重かったので、見た目の要点だけを1回の描画で作る
// 頂点シェーダーは MV1VertexShader.hlsl。BUMPMAP のときは VS_OUTPUT の並びが変わるので、同じマクロを付けてコンパイルする

Texture2D g_BaseTex : register(t0);		// 色(DxLib がマテリアルのテクスチャを入れる)
SamplerState g_BaseSampler : register(s0);
Texture2D g_NormalTex : register(t1);		// 法線マップ
SamplerState g_NormalSampler : register(s1);
Texture2D g_MaskTex : register(t2);		// マスク画像(R:金属 G:遮蔽 A:なめらかさ)
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

#define LIGHT_MAX 16

// ステージの明かり(LitModelPS.hlsl と同じ。ModelShaderManager が b4 に入れる)
cbuffer StageLightBuffer : register(b4)
{
	float4 g_LightPosRange[LIGHT_MAX];	// xyz:位置 w:届く距離
	float4 g_LightColor[LIGHT_MAX];		// rgb:色×強さ
	float4 g_Param;						// x:数 y:フォグの始まり z:フォグの終わり w:裏側への回り込み
};

// 光と、今描くマテリアルの値(CharacterLook::LookConstant と同じ並び)
cbuffer LookBuffer : register(b5)
{
	float4 g_KeyDir;		// xyz:光の来る向き(ワールド)
	float4 g_KeyColor;		// rgb:平行光の色
	float4 g_SkyColor;		// rgb:上からの環境光
	float4 g_GroundColor;	// rgb:下からの環境光
	float4 g_CameraPos;		// xyz:カメラの位置
	float4 g_Rim;			// rgb:ふちの光の色 w:ふちの細さ(大きいほど細い)
	float4 g_Mat0;			// x:法線の強さ y:遮蔽の強さ z:なめらかさの強さ w:金属の強さ
	float4 g_Mat1;			// x:法線マップあり y:マスクあり z:切り抜きのしきい値 w:光沢の強さ
	float4 g_Tint;			// rgb:色に掛ける w:法線マップの緑(Y)の向き(1 か -1)
};

static const float PI = 3.14159265f;

// 法線マップの向き(接線空間)を、モデルの接線データ無しで、近くのピクセルとの差(ddx/ddy)からワールドに直す
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

float4 main(PS_INPUT input) : SV_TARGET
{
	float2 uv = input.TexCoords0_1.xy;
	float4 base = g_BaseTex.Sample(g_BaseSampler, uv);
	float alpha = base.a * input.Diffuse.a;
	clip(alpha - g_Mat1.z);
	float3 albedo = base.rgb * g_Tint.rgb;

	// 面の向き
	float3 pos = input.WorldPosition;
	float3 normal = normalize(input.WorldNormal);
	if (g_Mat1.x > 0.5f)
	{
		float3 mapNormal = g_NormalTex.Sample(g_NormalSampler, uv).xyz * 2.0f - 1.0f;
		mapNormal.y *= g_Tint.w;
		mapNormal.xy *= g_Mat0.x;
		normal = ApplyNormalMap(normal, pos, uv, normalize(mapNormal));
	}

	// 遮蔽・なめらかさ・金属
	float ao = 1.0f;
	float smooth = 0.3f;
	float metal = 0.0f;
	if (g_Mat1.y > 0.5f)
	{
		float4 mask = g_MaskTex.Sample(g_MaskSampler, uv);
		ao = saturate(1.0f - (1.0f - mask.g) * g_Mat0.y);
		smooth = saturate(mask.a * g_Mat0.z);
		metal = saturate(mask.r * g_Mat0.w);
	}

	float3 toCamera = normalize(g_CameraPos.xyz - pos);
	float3 toKey = normalize(g_KeyDir.xyz);
	float nv = saturate(dot(normal, toCamera)) + 1e-4f;
	float3 diffuseColor = albedo * (1.0f - metal);
	float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metal);	// 真正面から見たときの反射の強さ
	float rough = max((1.0f - smooth) * (1.0f - smooth), 0.02f);

	// 平行光: 拡散 + 光沢(GGX)
	float nl = saturate(dot(normal, toKey));
	float3 halfVec = normalize(toKey + toCamera);
	float nh = saturate(dot(normal, halfVec));
	float denom = nh * nh * (rough * rough - 1.0f) + 1.0f;
	float distribution = rough * rough / (PI * denom * denom);
	float k = rough * 0.5f;
	float visibility = 0.25f / ((nl * (1.0f - k) + k) * (nv * (1.0f - k) + k));
	float3 fresnel = f0 + (1.0f - f0) * pow(1.0f - saturate(dot(toCamera, halfVec)), 5.0f);
	float3 color = (diffuseColor + distribution * visibility * fresnel * g_Mat1.w) * nl * g_KeyColor.rgb;

	// 環境光(上は空、下は地面の色)。光沢の分も少しだけ
	float3 ambient = lerp(g_GroundColor.rgb, g_SkyColor.rgb, normal.y * 0.5f + 0.5f);
	color += (diffuseColor + f0 * smooth * 0.5f) * ambient * ao;

	// ふちの光(夜でも形が分かるように)
	color += pow(1.0f - nv, g_Rim.w) * g_Rim.rgb * ao;

	// ステージの明かり
	int count = (int)g_Param.x;
	for (int i = 0; i < count; ++i)
	{
		float3 toLight = g_LightPosRange[i].xyz - pos;
		float dist = length(toLight);
		float atten = saturate(1.0f - dist / g_LightPosRange[i].w);
		atten *= atten;
		float ndl = dot(normal, toLight / max(dist, 0.001f));
		ndl = saturate(ndl * (1.0f - g_Param.w) + g_Param.w);
		color += albedo * g_LightColor[i].rgb * atten * ndl * ao;
	}

	return float4(color, alpha);
}
