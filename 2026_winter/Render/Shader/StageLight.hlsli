// ステージの明かり(点光源)と車内の箱。LitModelPS.hlsl と CharacterLookPS.hlsl で共通
// ModelShaderManager が b4 に入れる。並びは ModelShaderManager::StageLightConstant と同じ

#define LIGHT_MAX 16
#define INDOOR_MAX 4

cbuffer StageLightBuffer : register(b4)
{
	float4 g_LightPosRange[LIGHT_MAX];	// xyz:位置 w:届く距離の2乗(負なら車内だけ照らす)
	float4 g_LightColor[LIGHT_MAX];		// rgb:色×強さ w:1/届く距離(負なら外だけ照らす)
	float4 g_Param;						// x:数 y:車内の箱の数 z:(使っていない) w:裏側への回り込み
	float4 g_IndoorMin[INDOOR_MAX];		// 車内の箱(ワールド座標)
	float4 g_IndoorMax[INDOOR_MAX];
};

// 車内の箱の中なら1
float IsIndoor(float3 pos)
{
	int boxNum = (int)g_Param.y;
	for (int b = 0; b < boxNum; ++b)
	{
		if (all(pos >= g_IndoorMin[b].xyz) && all(pos <= g_IndoorMax[b].xyz)) return 1.0f;
	}
	return 0.0f;
}

// 明かりの色 × 距離の減り方 × 面の向き を足したもの(テクスチャの色は呼ぶ側で掛ける)
float3 CalcStageLights(float3 pos, float3 normal, float indoor)
{
	float3 lightSum = float3(0.0f, 0.0f, 0.0f);
	int count = (int)g_Param.x;
	for (int k = 0; k < count; ++k)
	{
		float rangeSq = g_LightPosRange[k].w;
		float invRange = g_LightColor[k].w;
		if (rangeSq < 0.0f && indoor < 0.5f) continue;	// 車内だけの明かり
		if (invRange < 0.0f && indoor > 0.5f) continue;	// 外だけの明かり
		float3 toPoint = g_LightPosRange[k].xyz - pos;
		float distSq = dot(toPoint, toPoint);
		if (distSq >= abs(rangeSq)) continue;	// 届かない明かりは計算しない
		float invDist = rsqrt(max(distSq, 1e-6f));		// 平方根と割り算の代わり
		float atten = saturate(1.0f - distSq * invDist * abs(invRange));
		atten *= atten;
		float ndl = saturate(dot(normal, toPoint * invDist) * (1.0f - g_Param.w) + g_Param.w);
		lightSum += g_LightColor[k].rgb * (atten * ndl);
	}
	return lightSum;
}
