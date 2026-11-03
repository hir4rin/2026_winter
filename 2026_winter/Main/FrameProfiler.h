#pragma once
#include "DxLib.h"
#include <array>
#include <vector>

/// <summary>
/// 1フレームの中の区切りごとの時間を測る(カクつきの原因を探す用)
/// kSpikeMs を超えたフレームは、内訳を Log/FrameSpikes.txt に書く。F2 の「Frame Time」ウィンドウでも見られる
/// </summary>
class FrameProfiler
{
private:
	FrameProfiler() = default;
	~FrameProfiler() = default;
	FrameProfiler(const FrameProfiler&) = delete;
	FrameProfiler& operator=(const FrameProfiler&) = delete;
public:
	static FrameProfiler& GetInstance()
	{
		static FrameProfiler instance;
		return instance;
	}

	/// <summary>フレームの始めに呼ぶ</summary>
	void BeginFrame();
	/// <summary>区切り。前の区切り(またはフレームの始め)からここまでの時間を name で覚える。name は文字列リテラルにすること</summary>
	void Mark(const char* name);
	/// <summary>フレームの終わり(ScreenFlip の後、60fps に合わせて待つ前)に呼ぶ</summary>
	void EndFrame();
	/// <summary>ImGui のウィンドウ(F2 の表示中)</summary>
	void DrawDebugWindow();
	/// <summary>
	/// 今がモニターの何回目の更新か(ScreenFlip の後に呼ぶ)
	/// 前のフレームから2回以上進んだ = 同じ絵が2回出た、0回 = 出なかった絵がある(どちらも見た目のカクッ)
	/// </summary>
	void SetRefreshIndex(LONGLONG refreshIndex);

private:
	struct Section
	{
		const char* name;
		float ms;
	};
	//区切りごとの集計(kStatFrame フレームごとに Log/FrameStats.txt に平均と最大を書く)
	struct Stat
	{
		const char* name;
		float sum;
		float min;
		float max;
	};
	static constexpr float kSpikeMs = 20.0f;//これより長いフレームを記録する(60fps は 16.7ms)
	static constexpr int kHistoryNum = 180;//グラフに出すフレーム数(3秒)
	static constexpr int kStatFrame = 60;

	void WriteSpike(float totalMs);
	void AddStat(const char* name, float ms);
	void WriteStats();

	std::vector<Stat> m_stats;
	int m_statCount = 0;
	bool m_isStatFileStarted = false;

	LONGLONG m_frameStart = 0;
	LONGLONG m_lastMark = 0;
	std::vector<Section> m_sections;//今のフレーム
	std::vector<Section> m_lastSections;//前のフレーム(ウィンドウに出す)
	float m_lastTotalMs = 0.0f;
	std::array<float, kHistoryNum> m_history = {};
	int m_historyIndex = 0;
	int m_frameCount = 0;
	int m_spikeCount = 0;
	bool m_isFileStarted = false;
	LONGLONG m_lastRefreshIndex = -1;
	int m_refreshMissCount = 0;//モニターの更新と1対1にならなかった回数
};
