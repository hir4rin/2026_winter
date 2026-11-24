// HPが低いときに画面の端から墨がにじむ(DrawPrimitive2DToShader 用。案は tools/LowHpMock の3)
// pass 0: 乗算(境目を赤黒く・墨の分だけ暗く)  pass 1: 加算(赤のにじみと墨の色)
// 2回に分けているのは、画面の色を読まずにモックと同じ式にするため

// DxLib の 2D 用頂点シェーダーの出力と同じ並び
struct PS_INPUT
{
	float4 Position        : SV_POSITION;
	float4 Diffuse         : COLOR0;
	float4 Specular        : COLOR1;
	float2 TexCoords0      : TEXCOORD0;
	float2 TexCoords1      : TEXCOORD1;
};

// LowHpInkUI が b10 に入れる
cbuffer LowHpInkBuffer : register(b10)
{
	float4 g_Param;		// x: 効き具合(0〜1)  y: 心拍(0〜1)  z: 時間(秒)  w: パス
	float4 g_Screen;	// x: 横/縦
};

static const float kStartEdge = 1.10f;		// 効き始めの墨の位置(中心0・四隅1)
static const float kMaxEdge = 0.66f;		// 一番強いときの墨の位置
static const float kBeatPush = 0.05f;		// 心拍で押し寄せる量
static const float kInkSoft = 0.045f;		// 墨のふちのぼかし
static const float kInkRate = 0.92f;		// 墨の濃さ
static const float kFringeWidth = 0.11f;	// 赤くにじむ幅
static const float kFringeRate = 0.7f;		// 赤くにじむ強さ
static const float kNoiseReach = 0.31f;		// ノイズで墨が内側へ出る最大
static const float3 kFringeMul = float3(0.55f, 0.12f, 0.14f);
static const float3 kFringeAdd = float3(0.13f, 0.0f, 0.015f);
static const float3 kInkColor = float3(0.028f, 0.010f, 0.016f);

float Hash(float2 p)
{
	p = frac(p * float2(123.34f, 456.21f));
	p += dot(p, p + 45.32f);
	return frac(p.x * p.y);
}

float Noise(float2 p)
{
	float2 i = floor(p);
	float2 f = frac(p);
	float2 u = f * f * (3.0f - 2.0f * f);
	float a = Hash(i);
	float b = Hash(i + float2(1.0f, 0.0f));
	float c = Hash(i + float2(0.0f, 1.0f));
	float d = Hash(i + float2(1.0f, 1.0f));
	return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}

float Fbm(float2 p)
{
	float v = 0.0f;
	float a = 0.5f;
	[unroll]
	for (int i = 0; i < 5; ++i)
	{
		v += a * Noise(p);
		p = p * 2.03f + float2(1.7f, 9.2f);
		a *= 0.5f;
	}
	return v;
}

float4 main(PS_INPUT input) : SV_TARGET0
{
	float k = g_Param.x;
	float aspect = g_Screen.x;
	float2 uv = input.TexCoords0;

	// 中心0・四隅1
	float2 c = (uv - 0.5f) * float2(aspect, 1.0f);
	float e = length(c) / length(float2(aspect, 1.0f) * 0.5f);
	float th = lerp(kStartEdge, kMaxEdge, k) - kBeatPush * g_Param.y * k;

	// 墨が届かない所は何もしない
	if (e < th - kFringeWidth - kNoiseReach) discard;

	float t = g_Param.z;
	float2 p = uv * float2(aspect, 1.0f) * 2.2f;
	float n = Fbm(p + float2(0.0f, t * 0.035f));
	float n2 = Fbm(p * 4.0f - float2(t * 0.05f, 0.0f));
	float field = e + (n - 0.5f) * 0.48f + (n2 - 0.5f) * 0.12f;

	float ink = smoothstep(th, th + kInkSoft, field);
	float fringe = smoothstep(th - kFringeWidth, th, field) * (1.0f - ink) * kFringeRate;
	float m = ink * kInkRate;

	if (g_Param.w < 0.5f)
	{
		float3 mul = lerp(float3(1.0f, 1.0f, 1.0f), kFringeMul, fringe) * (1.0f - m);
		return float4(mul, 1.0f);
	}
	float3 inkColor = kInkColor + (n2 - 0.5f) * 0.03f;
	float3 add = kFringeAdd * fringe * (1.0f - m) + inkColor * m;
	return float4(max(add, 0.0f), 0.0f);
}
