#pragma once
#include "Math/Vector3.h"
#include "Render/ModelShaderManager.h"
#include <utility>
#include <vector>

/// <summary>
/// ステージの明かり(data/Stage/Lights/StageXXX.csv。UnityのCyberStageToolで書き出す)
/// プレイヤーに近いものを ModelShaderManager に渡し、いつもの光と一緒に1回で描く(LitModelPS.hlsl)
/// </summary>
class StageLights
{
public:
	StageLights() = default;
	~StageLights();

	/// <summary>CSVを読む。ファイルが無ければ何もしない</summary>
	/// <param name="offset">ステージのモデルをずらした量(明かりの位置にも足す)</param>
	void Load(int stageNumber, const Vector3& offset);
	/// <summary>遠くの光を薄くする距離(ステージのフォグに合わせる)</summary>
	void SetFogRange(float start, float end) { m_fogStart = start; m_fogEnd = end; }
	/// <summary>プレイヤーに近い明かりをシェーダーに渡す</summary>
	void Update(const Vector3& playerPos);
	/// <summary>強さなどを変えるImGuiウィンドウ(明かりが無いステージでは出さない)</summary>
	void DrawDebugWindow();

private:
	struct LightData
	{
		Vector3 pos;
		float r = 1.0f, g = 1.0f, b = 1.0f;
		float range = 0.0f;//届く距離
		float intensity = 1.0f;
	};

	void Release();

	std::vector<LightData> m_lights;
	std::vector<std::pair<float, int>> m_nearBuffer;//Update で使い回す(距離, 番号)
	std::vector<ModelShaderManager::StageLightParam> m_paramBuffer;
	int m_activeNum = 0;//今渡している数(デバッグ表示用)

	bool m_isEnable = true;
	float m_colorScale = 3.0f;//光の強さの倍率
	float m_activeDistance = 4000.0f;//プレイヤーからこの距離までの明かりを使う
	float m_fadeDistance = 1000.0f;//遠い端のこの幅でだんだん暗くする(入れ替わりでパッと変わらないように)
	float m_wrap = 0.2f;//光に背を向けた面もこの割合だけ照らす
	float m_fogStart = 100000.0f;//フォグの無いステージでは薄くしない
	float m_fogEnd = 200000.0f;
};
