#pragma once
#include "DxLib.h"
#include <array>
#include <condition_variable>
#include <cstdarg>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/// <summary>
/// 1フレームの中の区切りごとの時間を測る(カクつきの原因を探す用)
/// kSpikeMs を超えたフレームは、内訳を Log/FrameSpikes.txt に書く。F2 の「Frame Time」ウィンドウでも見られる
/// ログはメモリにためて、別のスレッドが kLogIntervalMs ごとにファイルへ書く(書き込みでゲームが止まらないように)
/// </summary>
class FrameProfiler
{
private:
	FrameProfiler() = default;
	~FrameProfiler();
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
	/// <summary>出来事を Log/CameraEvents.txt に書く(printf と同じ書き方)。フレーム番号は FrameSpikes.txt と同じ</summary>
	void LogEvent(const char* format, ...);
	int GetFrameCount()const { return m_frameCount; }
	/// <summary>モニターの1回の更新の長さ(ms)。フレームの間隔がこれから kPeriodJitterMs 以上ずれたら記録する</summary>
	void SetExpectedPeriod(float ms) { m_expectedPeriodMs = ms; }
	/// <summary>ゲームのスレッドの優先度を上げる(待ちから戻るのが遅れるのを防げるか試す用。F2 の「Frame Time」か起動の引数 -prio=high)</summary>
	void SetHighPriority(bool isHigh);
	/// <summary>残りのログをファイルに書いて、書き込みのスレッドを止める(終了時に呼ぶ)</summary>
	void Terminate();

private:
	enum class LogFile { Spike, Stat, Event, Num };
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
	static constexpr float kPeriodJitterMs = 3.0f;//フレームの間隔がこれ以上ずれたら記録する
	static constexpr float kMemoryBarMaxGB = 4.0f;//メモリのバーがいっぱいになる量
	static constexpr int kLogIntervalMs = 1000;//ログをファイルに書く間隔
	static constexpr int kLogTextMax = 512;//1回に足せるログの長さ
	static constexpr int kLogFileNum = static_cast<int>(LogFile::Num);

	//処理の重さ(1フレーム分に対して)とメモリのバー
	void DrawLoadBar();
	void DrawMemoryBar();
	void WriteSpike(float totalMs);
	void WritePeriodJitter(float periodMs);
	void AddStat(const char* name, float ms);
	void WriteStats();
	//ログをメモリに足す(printf と同じ書き方)
	void AddLog(LogFile file, const char* format, ...);
	void AddLogV(LogFile file, const char* format, va_list args);
	void AddSectionLog();
	//書き込みのスレッド
	void LogThread();
	void WriteLogFiles();

	std::vector<Stat> m_stats;
	int m_statCount = 0;

	std::thread m_logThread;
	std::mutex m_logMutex;
	std::condition_variable m_logWake;
	std::array<std::string, kLogFileNum> m_logBuffers;//ゲームのスレッドがためる
	std::array<std::string, kLogFileNum> m_logWriting;//書き込みのスレッドだけが使う
	std::array<bool, kLogFileNum> m_isLogFileStarted = {};//書き込みのスレッドだけが使う
	bool m_isLogStop = false;

	LONGLONG m_frameStart = 0;
	LONGLONG m_lastMark = 0;
	std::vector<Section> m_sections;//今のフレーム
	std::vector<Section> m_lastSections;//前のフレーム(ウィンドウに出す)
	float m_lastTotalMs = 0.0f;
	std::array<float, kHistoryNum> m_history = {};
	int m_historyIndex = 0;
	int m_frameCount = 0;
	int m_spikeCount = 0;
	LONGLONG m_lastRefreshIndex = -1;
	int m_refreshMissCount = 0;//モニターの更新と1対1にならなかった回数
	float m_expectedPeriodMs = 1000.0f / 60.0f;
	int m_jitterCount = 0;//間隔がずれた回数
	float m_lastPeriodMs = 0.0f;
	float m_fps = 0.0f;//1秒ごとの平均
	float m_fpsTimeMs = 0.0f;
	int m_fpsFrameNum = 0;
	bool m_isHighPriority = false;//ゲームのスレッドの優先度を上げているか(待ちから戻るのが遅れるのを防ぐ)
};
