// ステージ・敵を「DxLib のいつもの光 ＋ ステージの明かり」で1回で描くピクセルシェーダー
// 前は いつもの描画の上に明かりの光を加算でもう1回描いていたが、1回目(DxLib の標準)と2回目で深さがほんの少しずれて床がちかちかした
// 1回で描けば深さのずれは起きない。DxLib の光の値(ライト・マテリアル・フォグ)は共通の定数バッファ(b0)から読む
// 頂点シェーダーは MV1VertexShader.hlsl。BUMPMAP のときは VS_OUTPUT の並びが変わるので、同じマクロを付けてコンパイルする

#include "DataType.h"
#include "DxShader_Common_D3D11.h"

// DxLib が頂点シェーダー・ピクセルシェーダーの両方に入れている共通の値
cbuffer cbD3D11_CONST_BUFFER_COMMON : register(b0)
{
	DX_D3D11_CONST_BUFFER_COMMON g_Common;
};

Texture2D g_DiffuseMapTexture : register(t0);
SamplerState g_DiffuseMapSampler : register(s0);

// MV1VertexShader.hlsl の VS_OUTPUT と同じ並び
struct PS_INPUT
{
	float4 Diffuse         : COLOR0;		// マテリアルの拡散の色
	float4 Specular        : COLOR1;		// マテリアルの反射の色
	float4 TexCoords0_1    : TEXCOORD0;
	float3 VPosition       : POSITION0;		// 位置(ビュー空間)
	float3 VNormal         : NORMAL0;		// 法線(ビュー空間)
#ifdef BUMPMAP
	float3 VTan            : TANGENT;
	float3 VBin            : BINORMAL;
#endif
	float2 Fog             : TEXCOORD5;
	float4 Position        : SV_POSITION;
	float3 WorldPosition   : Position1;
	float3 WorldNormal     : Normal1;
};

// ステージの明かり(b4)
#include "StageLight.hlsli"

// ステージの空気感(列車ステージなど。ModelShaderManager が b10 に入れる)
// いつもの光の環境光を、面の向き(上・下)・平行光・横からの光で色分けする
cbuffer AtmosphereBuffer : register(b10)
{
	float4 g_AtmoSky;			// rgb:上を向いた面の倍率 a:1なら使う
	float4 g_AtmoGround;		// rgb:下を向いた面の倍率
	float4 g_AtmoIndoorSky;		// rgb:車内の上を向いた面の倍率
	float4 g_AtmoIndoorGround;	// rgb:車内の下を向いた面の倍率
	float4 g_AtmoKeyDir;		// xyz:平行光の来る向き
	float4 g_AtmoKeyColor;		// rgb:平行光の倍率
	float4 g_AtmoSideDir;		// xyz:横からの光の来る向き
	float4 g_AtmoSideColor;		// rgb:横からの光の倍率(車内では使わない)
};

// ステージの色変え(ModelShaderManager が b7 に入れる)
cbuffer ColorGradeBuffer : register(b7)
{
	float4 g_GradeDark;		// rgb:暗い所の色 a:かかり具合
	float4 g_GradeBright;	// rgb:明るい所の色
	float4 g_GradeLevel;	// x:これより暗いと暗い所の色 y:これより明るいと明るい所の色
};

// ガード中のふちの光(ModelShaderManager が b8 に入れる)
cbuffer GuardRimBuffer : register(b8)
{
	float4 g_GuardRimColor;	// rgb:色×強さ(0なら光らない)
	float4 g_GuardRimParam;	// x:ふちの細さ(大きいほど細い)
};

#define LIGHTTYPE_DIRECTIONAL 3


float4 main(PS_INPUT input) : SV_TARGET
{
	float4 tex = g_DiffuseMapTexture.Sample(g_DiffuseMapSampler, input.TexCoords0_1.xy);
	float alpha = tex.a * input.Diffuse.a;
	clip(alpha - 0.004f);

	// DxLib のいつもの光(平行光だけ。このゲームは平行光しか使っていない)
	float3 normal = normalize(input.VNormal);
	float3 toCamera = normalize(-input.VPosition);
	float3 diffuse = float3(0.0f, 0.0f, 0.0f);
	float3 specular = float3(0.0f, 0.0f, 0.0f);
	float3 emissive = g_Common.Material.Ambient_Emissive.rgb;	// 発光 + マテリアルの環境光 × 全体の環境光
	float3 lightAmbient = float3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < DX_D3D11_COMMON_CONST_LIGHT_NUM; ++i)
	{
		if (g_Common.Light[i].Type != LIGHTTYPE_DIRECTIONAL) continue;
		float3 toLight = -normalize(g_Common.Light[i].Direction);
		float nl = saturate(dot(normal, toLight));
		diffuse += g_Common.Light[i].Diffuse * nl;
		float3 halfVec = normalize(toLight + toCamera);
		specular += g_Common.Light[i].Specular * pow(saturate(dot(normal, halfVec)), max(g_Common.Material.Power, 1.0f)) * (nl > 0.0f ? 1.0f : 0.0f);
		lightAmbient += g_Common.Light[i].Ambient.rgb;			// ライトの環境光 × マテリアルの環境光
	}

	float3 worldNormal = normalize(input.WorldNormal);
	float indoor = IsIndoor(input.WorldPosition);

	// 空気感: 環境光(=マテリアルの明るさ)を面の向きで色分けする
	if (g_AtmoSky.a > 0.5f)
	{
		float up = worldNormal.y * 0.5f + 0.5f;
		float3 hemi = lerp(lerp(g_AtmoGround.rgb, g_AtmoSky.rgb, up), lerp(g_AtmoIndoorGround.rgb, g_AtmoIndoorSky.rgb, up), indoor);
		float key = saturate(dot(worldNormal, g_AtmoKeyDir.xyz));
		float side = saturate(dot(worldNormal, g_AtmoSideDir.xyz));
		lightAmbient *= hemi + g_AtmoKeyColor.rgb * key + g_AtmoSideColor.rgb * (side * (1.0f - indoor));
	}
	float3 color = tex.rgb * saturate(diffuse * input.Diffuse.rgb + emissive + lightAmbient) + specular * input.Specular.rgb;

	// ステージの明かり(足してからテクスチャの色を1回掛ける)
	color += tex.rgb * CalcStageLights(input.WorldPosition, worldNormal, indoor);

	// フォグ(頂点シェーダーが Fog.x に計算済み。フォグが無いときは 1)
	color = lerp(g_Common.Fog.Color.rgb, color, saturate(input.Fog.x * input.Fog.y));

	// ガード中のふちの光(フォグの後に足して、遠くでも見えるようにする)
	if (g_GuardRimColor.r + g_GuardRimColor.g + g_GuardRimColor.b > 0.0f)
	{
		float nv = saturate(dot(normal, toCamera));
		color += pow(1.0f - nv, g_GuardRimParam.x) * g_GuardRimColor.rgb;
	}

	// 色変え(フォグの色も一緒に変える)
	if (g_GradeDark.a > 0.0f)
	{
		float luma = dot(color, float3(0.299f, 0.587f, 0.114f));
		luma = saturate((luma - g_GradeLevel.x) / (g_GradeLevel.y - g_GradeLevel.x));
		color = lerp(color, lerp(g_GradeDark.rgb, g_GradeBright.rgb, luma), g_GradeDark.a);
	}

	return float4(color, alpha);
}
