// 血殺中に敵を真っ黒にするピクセルシェーダー
// 2026_summer の EnemyBlackPS.hlsl をもとにしている
// 頂点シェーダーは MV1VertexShader.hlsl(頂点タイプごとにマクロを変えてコンパイルしたもの)を使う

// テクスチャ(DxLibがモデルのディフューズテクスチャを t0 / s0 にセットする)
Texture2D g_DiffuseMapTexture : register(t0);
SamplerState g_DiffuseMapSampler : register(s0);

// MV1VertexShader.hlsl の VS_OUTPUT の「先頭から」同じ並びにする
// (D3D11では頂点シェーダーの出力とピクセルシェーダーの入力の並びが合っていないと値がずれるため)
// VS_OUTPUT は BUMPMAP のときだけ途中に接線・従法線が入るので、どの頂点タイプでも並びが同じ先頭3つだけ受け取る
struct PS_INPUT
{
	float4 Diffuse         : COLOR0;		// ディフューズカラー
	float4 Specular        : COLOR1;		// スペキュラカラー
	float4 TexCoords0_1    : TEXCOORD0;		// xy:テクスチャ座標 zw:サブテクスチャ座標
};

// 黒の色(真っ黒より少しだけ明るくしたいときはここを変える)
static const float3 kBlackColor = float3(0.0f, 0.0f, 0.0f);

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 texColor = g_DiffuseMapTexture.Sample(g_DiffuseMapSampler, input.TexCoords0_1.xy);

	// 透明な部分は透明のまま残す(シルエットの形を保つ)
	float alpha = texColor.a * input.Diffuse.a;
	clip(alpha - 0.01f);

	return float4(kBlackColor, alpha);
}
