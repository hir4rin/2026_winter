#include "FrameProfiler.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace
{
	const char* const kSpikeLogPath = "Log/FrameSpikes.txt";
	const char* const kStatLogPath = "Log/FrameStats.txt";
}

void FrameProfiler::BeginFrame()
{
	const LONGLONG now = GetNowHiPerformanceCount();
	//前のフレームの始まりからの間隔(待ちも含む。60fps なら 16.7ms で、ばらつくとカクついて見える)
	if (m_frameStart != 0) AddStat("PERIOD", static_cast<float>(now - m_frameStart) / 1000.0f);
	m_frameStart = now;
	m_lastMark = m_frameStart;
	m_sections.clear();
}

void FrameProfiler::Mark(const char* name)
{
	const LONGLONG now = GetNowHiPerformanceCount();
	m_sections.push_back({ name, static_cast<float>(now - m_lastMark) / 1000.0f });
	m_lastMark = now;
}

void FrameProfiler::EndFrame()
{
	const float totalMs = static_cast<float>(GetNowHiPerformanceCount() - m_frameStart) / 1000.0f;
	m_lastTotalMs = totalMs;
	m_lastSections.swap(m_sections);
	m_history[m_historyIndex] = totalMs;
	m_historyIndex = (m_historyIndex + 1) % kHistoryNum;
	++m_frameCount;
	if (totalMs > kSpikeMs) WriteSpike(totalMs);

	//集計
	for (const auto& section : m_lastSections) AddStat(section.name, section.ms);
	AddStat("TOTAL", totalMs);
	if (++m_statCount >= kStatFrame) WriteStats();
}

void FrameProfiler::AddStat(const char* name, float ms)
{
	for (auto& stat : m_stats)
	{
		if (stat.name == name)
		{
			stat.sum += ms;
			stat.min = (std::min)(stat.min, ms);
			stat.max = (std::max)(stat.max, ms);
			return;
		}
	}
	m_stats.push_back({ name, ms, ms, ms });
}

void FrameProfiler::WriteStats()
{
	if (!m_isStatFileStarted) std::filesystem::create_directories("Log");
	FILE* file = nullptr;
	if (fopen_s(&file, kStatLogPath, m_isStatFileStarted ? "a" : "w") == 0 && file != nullptr)
	{
		m_isStatFileStarted = true;
		fprintf(file, "frame %d (avg/min/max ms):", m_frameCount);
		for (const auto& stat : m_stats) fprintf(file, " %s=%.2f/%.1f/%.1f", stat.name, stat.sum / m_statCount, stat.min, stat.max);
		fprintf(file, "\n");
		fclose(file);
	}
	m_stats.clear();
	m_statCount = 0;
}

void FrameProfiler::WriteSpike(float totalMs)
{
	++m_spikeCount;
	//起動して最初の記録のときにファイルを作り直す
	if (!m_isFileStarted) std::filesystem::create_directories("Log");
	FILE* file = nullptr;
	if (fopen_s(&file, kSpikeLogPath, m_isFileStarted ? "a" : "w") != 0 || file == nullptr) return;
	m_isFileStarted = true;
	fprintf(file, "frame %d total %.1fms:", m_frameCount, totalMs);
	for (const auto& section : m_lastSections) fprintf(file, " %s=%.1f", section.name, section.ms);
	fprintf(file, "\n");
	fclose(file);
}

void FrameProfiler::SetRefreshIndex(LONGLONG refreshIndex)
{
	if (m_lastRefreshIndex >= 0)
	{
		const LONGLONG delta = refreshIndex - m_lastRefreshIndex;
		AddStat("REFRESH", static_cast<float>(delta));
		if (delta != 1)
		{
			++m_refreshMissCount;
			if (!m_isFileStarted) std::filesystem::create_directories("Log");
			FILE* file = nullptr;
			if (fopen_s(&file, kSpikeLogPath, m_isFileStarted ? "a" : "w") == 0 && file != nullptr)
			{
				m_isFileStarted = true;
				fprintf(file, "frame %d refresh +%lld (1 が正常)\n", m_frameCount, delta);
				fclose(file);
			}
		}
	}
	m_lastRefreshIndex = refreshIndex;
}

void FrameProfiler::DrawDebugWindow()
{
	ImGui::Begin("Frame Time");
	float worst = 0.0f;
	for (float ms : m_history) worst = (std::max)(worst, ms);
	ImGui::Text("Last %.1f ms / Worst(3s) %.1f ms / Spikes(>%.0fms) %d", m_lastTotalMs, worst, kSpikeMs, m_spikeCount);
	ImGui::Text("Refresh miss %d", m_refreshMissCount);
	ImGui::PlotLines("##history", m_history.data(), kHistoryNum, m_historyIndex, nullptr, 0.0f, 40.0f, ImVec2(0, 60));
	for (const auto& section : m_lastSections) ImGui::Text("%-14s %5.2f ms", section.name, section.ms);
	ImGui::End();
}
