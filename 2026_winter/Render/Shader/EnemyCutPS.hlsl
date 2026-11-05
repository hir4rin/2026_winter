// トドメで敵を真っ二つにするピクセルシェーダー
// EnemyBlackPS と同じ黒いシルエットで、面の裏側を捨てる。切り口のそばは白く光らせる
// 頂点シェーダーは MV1VertexShader.hlsl。BUMPMAP のときは VS_OUTPUT の並びが変わるので、同じマクロを付けてコンパイルする

Texture2D g_DiffuseMapTexture : register(t0);
SamplerState g_DiffuseMapSampler : register(s0);

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

// ModelShaderManager が b6 に入れる
cbuffer CutBuffer : register(b6)
{
	float4 g_CutPlane;	// xyz:法線 w:原点からの距離
	float4 g_CutParam;	// x:切り口の光の幅
};

float4 main(PS_INPUT input) : SV_TARGET
{
	float4 tex = g_DiffuseMapTexture.Sample(g_DiffuseMapSampler, input.TexCoords0_1.xy);
	float alpha = tex.a * input.Diffuse.a;
	clip(alpha - 0.01f);

	float planeDist = dot(input.WorldPosition, g_CutPlane.xyz) - g_CutPlane.w;
	clip(planeDist);

	float rim = 1.0f - saturate(planeDist / g_CutParam.x);
	return float4(rim, rim, rim, alpha);
}
