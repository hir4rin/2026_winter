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

#define LIGHT_MAX 16

// ステージの明かり(ModelShaderManager が b4 に入れる)
cbuffer StageLightBuffer : register(b4)
{
	float4 g_LightPosRange[LIGHT_MAX];	// xyz:位置 w:届く距離
	float4 g_LightColor[LIGHT_MAX];		// rgb:色×強さ
	float4 g_Param;						// x:数 y,z:(使っていない) w:裏側への回り込み
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
	float3 ambient = g_Common.Material.Ambient_Emissive.rgb;	// 発光 + マテリアルの環境光 × 全体の環境光
	for (int i = 0; i < DX_D3D11_COMMON_CONST_LIGHT_NUM; ++i)
	{
		if (g_Common.Light[i].Type != LIGHTTYPE_DIRECTIONAL) continue;
		float3 toLight = -normalize(g_Common.Light[i].Direction);
		float nl = saturate(dot(normal, toLight));
		diffuse += g_Common.Light[i].Diffuse * nl;
		float3 halfVec = normalize(toLight + toCamera);
		specular += g_Common.Light[i].Specular * pow(saturate(dot(normal, halfVec)), max(g_Common.Material.Power, 1.0f)) * (nl > 0.0f ? 1.0f : 0.0f);
		ambient += g_Common.Light[i].Ambient.rgb;				// ライトの環境光 × マテリアルの環境光
	}
	float3 color = tex.rgb * saturate(diffuse * input.Diffuse.rgb + ambient) + specular * input.Specular.rgb;

	// ステージの明かり(テクスチャの色 × 明かりの色 × 距離の減り方 × 面の向き)
	float3 worldNormal = normalize(input.WorldNormal);
	int count = (int)g_Param.x;
	for (int k = 0; k < count; ++k)
	{
		float3 toPoint = g_LightPosRange[k].xyz - input.WorldPosition;
		float dist = length(toPoint);
		float atten = saturate(1.0f - dist / g_LightPosRange[k].w);
		atten *= atten;
		float ndl = dot(worldNormal, toPoint / max(dist, 0.001f));
		ndl = saturate(ndl * (1.0f - g_Param.w) + g_Param.w);
		color += tex.rgb * g_LightColor[k].rgb * atten * ndl;
	}

	// フォグ(頂点シェーダーが Fog.x に計算済み。フォグが無いときは 1)
	color = lerp(g_Common.Fog.Color.rgb, color, saturate(input.Fog.x * input.Fog.y));

	return float4(color, alpha);
}
