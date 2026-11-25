// RTで次の目的地を見ている間の画面効果(DrawPrimitive2DToShader 用。案は tools/LookNextMock/postfx_mock.html の C)
// 色を抜いて目的地の周りだけ残す＋墨のふち＋入りだけ目的地へ吸い込む放射ブラー

// DxLib の 2D 用頂点シェーダーの出力と同じ並び
struct PS_INPUT
{
	float4 Position        : SV_POSITION;
	float4 Diffuse         : COLOR0;
	float4 Specular        : COLOR1;
	float2 TexCoords0      : TEXCOORD0;
	float2 TexCoords1      : TEXCOORD1;
};

Texture2D g_ScreenTexture : register(t0);// 今の画面の写し
SamplerState g_ScreenSampler : register(s0);
Texture2D g_InkTexture : register(t1);// 墨のビネット(アルファが濃さ)
SamplerState g_InkSampler : register(s1);

// LookNextFxUI が b11 に入れる
cbuffer LookNextFxBuffer : register(b11)
{
	float4 g_Param;		// x: 効き具合(0〜1)  y: 入りのブラー(0〜1)  z: 横/縦
	float4 g_Target;	// xy: 目的地(UV)
};

static const float kSaturation = 0.08f;	// 周りの色の残り
static const float kDark = 0.80f;		// 周りの明るさ
static const float kRadius = 0.22f;		// 色が残る円(画面の高さが1)
static const float kSoft = 0.55f;		// 円のふちのぼけ
static const float kBoost = 1.15f;		// 円の中の明るさ
static const float kInkRate = 0.55f;	// 墨のふちの濃さ
static const float kBlur = 0.10f;		// 入りの放射ブラーの長さ
static const int kBlurSample = 16;

float4 main(PS_INPUT input) : SV_TARGET0
{
	float2 uv = input.TexCoords0;
	float2 target = g_Target.xy;
	float3 orig = g_ScreenTexture.Sample(g_ScreenSampler, uv).rgb;

	float dist = length((uv - target) * float2(g_Param.z, 1.0f));
	float inside = 1.0f - smoothstep(kRadius * (1.0f - kSoft), kRadius * (1.0f + kSoft), dist);

	// 入りの放射ブラー(目的地へ吸い込む)
	float3 c = orig;
	float blur = kBlur * g_Param.y;
	if (blur > 0.0001f)
	{
		float3 sum = 0.0f;
		[unroll]
		for (int i = 0; i < kBlurSample; ++i)
		{
			float t = i / (float)(kBlurSample - 1);
			sum += g_ScreenTexture.Sample(g_ScreenSampler, lerp(uv, target, t * blur * (1.0f - inside * 0.7f))).rgb;
		}
		c = sum / kBlurSample;
	}

	float l = dot(c, float3(0.299f, 0.587f, 0.114f));
	float3 outer = lerp(float3(l, l, l), c, kSaturation) * kDark;
	float3 col = lerp(outer, c * kBoost, inside);

	// 墨のふち
	col *= 1.0f - g_InkTexture.Sample(g_InkSampler, uv).a * kInkRate;

	return float4(lerp(orig, col, g_Param.x), 1.0f);
}
