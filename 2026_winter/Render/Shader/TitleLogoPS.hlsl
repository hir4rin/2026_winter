// タイトルのロゴ「忍」を斬って、下半分を赤く染めて垂らすピクセルシェーダー(DrawPrimitive2DToShader 用)
// t0: 斬る前の鋼 / t1: 同じ形の赤。どちらも乗算済みアルファ
// 座標は画像のピクセル(y は下向き)。板は画像より下に長く、v が 1 を超えた所は垂れだけが入る

// DxLib の 2D 用頂点シェーダーの出力と同じ並び(先頭の位置を抜くと全部ずれる)
struct PS_INPUT
{
	float4 Position        : SV_POSITION;
	float4 Diffuse         : COLOR0;
	float4 Specular        : COLOR1;
	float2 TexCoords0      : TEXCOORD0;
	float2 TexCoords1      : TEXCOORD1;
};

Texture2D g_SteelTexture : register(t0);
SamplerState g_SteelSampler : register(s0);
Texture2D g_RedTexture : register(t1);
SamplerState g_RedSampler : register(s1);

// TitleScene が b9 に入れる
cbuffer TitleLogoBuffer : register(b9)
{
	float4 g_Size;	// xy: 画像の大きさ  zw: 1/大きさ
	float4 g_Line;	// xy: 斬り線の上の点  zw: 線の向き
	float4 g_Cut;	// x: 上のずれ  y: すき間  z: 赤く染まる進み(0〜1)  w: 垂れの進み(0〜1)
	float4 g_Drip;	// x: 垂れの最大の長さ  y: 垂れの間隔  z: 垂れの太さ  w: 染まりのぼかし
};

static const int kDripStep = 32;	// 垂れを探す回数
static const float kSolidAlpha = 0.7f;	// これより濃い所から垂れる(ふちのぼかしからは垂れない)

float Hash(float n)
{
	return frac(sin(n * 12.9898f) * 43758.5453f);
}

// 列 x の垂れの長さ。間隔ごとに1本、太さ・長さ・速さを乱数で変える。profile は垂れの真ん中ほど 1
float DripLength(float x, out float profile)
{
	float id = floor(x / g_Drip.y);
	float len = 0.0f;
	profile = 0.0f;
	[unroll]
	for (int i = -1; i <= 1; ++i)
	{
		float c = id + i;
		float has = step(0.4f, Hash(c * 1.7f));
		float center = (c + 0.2f + 0.6f * Hash(c * 3.1f)) * g_Drip.y;
		float width = g_Drip.z * (0.45f + Hash(c * 5.3f));
		float maxLen = g_Drip.x * (0.12f + 0.88f * pow(Hash(c * 7.9f), 1.6f));
		float t = saturate(g_Cut.w * (0.55f + 0.9f * Hash(c * 9.7f)));
		float grow = 1.0f - pow(1.0f - t, 2.5f);
		float d = (x - center) / width;
		float p = sqrt(saturate(1.0f - d * d));
		float l = has * maxLen * grow * p;
		if (l > len)
		{
			len = l;
			profile = p;
		}
	}
	return len;
}

float4 main(PS_INPUT input) : SV_TARGET0
{
	float2 p = input.TexCoords0 * g_Size.xy;
	float2 dir = g_Line.zw;
	float2 nrm = float2(-dir.y, dir.x);	// 線の下側が +
	float dist = dot(p - g_Line.xy, nrm);
	float halfGap = g_Cut.y * 0.5f;
	float fade = input.Diffuse.a;

	// 上: 線に沿ってずらす
	if (dist < 0.0f)
	{
		if (dist > -halfGap) return float4(0, 0, 0, 0);
		float2 src = p + dir * g_Cut.x;
		return g_SteelTexture.Sample(g_SteelSampler, src * g_Size.zw) * fade;
	}
	if (dist < halfGap) return float4(0, 0, 0, 0);

	// 下: 斬り口から下へ赤く染まる
	float2 uv = p * g_Size.zw;
	float4 steel = g_SteelTexture.Sample(g_SteelSampler, uv);
	float4 red = g_RedTexture.Sample(g_RedSampler, uv);
	float reach = g_Cut.z * g_Size.y;
	float redRate = saturate((reach - dist) / g_Drip.w);
	float4 col = lerp(steel, red, redRate);
	if (uv.y > 1.0f) col = float4(0, 0, 0, 0);

	// 垂れ: 上に赤い所があれば、この長さまで垂れてくる
	float profile;
	float len = DripLength(p.x, profile);
	if (len > 0.5f && col.a < 0.99f)
	{
		[loop]
		for (int i = 1; i <= kDripStep; ++i)
		{
			float off = len * i / kDripStep;
			float2 q = p - float2(0.0f, off);
			if (dot(q - g_Line.xy, nrm) < halfGap) break;
			float4 r = g_RedTexture.SampleLevel(g_RedSampler, q * g_Size.zw, 0);
			if (r.a > kSolidAlpha && q.y * g_Size.w <= 1.0f)
			{
				// 先へ行くほど暗い血の色
				float k = off / max(len, 1.0f);
				float3 blood = lerp(float3(0.62f, 0.02f, 0.06f), float3(0.30f, 0.0f, 0.02f), k);
				// 真ん中に細い光(ぬれた感じ)
				blood += float3(0.45f, 0.12f, 0.12f) * pow(profile, 14.0f) * (1.0f - k);
				float4 drip = float4(blood, 1.0f) * redRate;
				col = col + drip * (1.0f - col.a);
				break;
			}
		}
	}
	return col * fade;
}
