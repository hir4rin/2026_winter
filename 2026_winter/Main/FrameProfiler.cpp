#include "FrameProfiler.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <psapi.h>

namespace
{
	const char* const kSpikeLogPath = "Log/FrameSpikes.txt";
	const char* const kStatLogPath = "Log/FrameStats.txt";
	const char* const kEventLogPath = "Log/CameraEvents.txt";
	constexpr float kBytesPerGB = 1024.0f * 1024.0f * 1024.0f;
}

void FrameProfiler::LogEvent(const char* format, ...)
{
	//起動して最初の記録のときにファイルを作り直す
	if (!m_isEventFileStarted) std::filesystem::create_directories("Log");
	FILE* file = nullptr;
	if (fopen_s(&file, kEventLogPath, m_isEventFileStarted ? "a" : "w") != 0 || file == nullptr) return;
	m_isEventFileStarted = true;
	fprintf(file, "frame %d ", m_frameCount);
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
	fprintf(file, "\n");
	fclose(file);
}

void FrameProfiler::BeginFrame()
{
	const LONGLONG now = GetNowHiPerformanceCount();
	//前のフレームの始まりからの間隔(待ちも含む。60fps なら 16.7ms で、ばらつくとカクついて見える)
	if (m_frameStart != 0)
	{
		const float periodMs = static_cast<float>(now - m_frameStart) / 1000.0f;
		AddStat("PERIOD", periodMs);
		m_lastPeriodMs = periodMs;
		//1秒ごとに平均のfpsを出し直す
		m_fpsTimeMs += periodMs;
		++m_fpsFrameNum;
		if (m_fpsTimeMs >= 1000.0f)
		{
			m_fps = m_fpsFrameNum * 1000.0f / m_fpsTimeMs;
			m_fpsTimeMs = 0.0f;
			m_fpsFrameNum = 0;
		}
		if (std::abs(periodMs - m_expectedPeriodMs) >= kPeriodJitterMs) WritePeriodJitter(periodMs);
	}
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

void FrameProfiler::WritePeriodJitter(float periodMs)
{
	++m_jitterCount;
	//m_lastSections はまだ前のフレームの内訳
	if (!m_isFileStarted) std::filesystem::create_directories("Log");
	FILE* file = nullptr;
	if (fopen_s(&file, kSpikeLogPath, m_isFileStarted ? "a" : "w") != 0 || file == nullptr) return;
	m_isFileStarted = true;
	fprintf(file, "frame %d period %.1fms%s:", m_frameCount - 1, periodMs, m_isHighPriority ? " (high)" : "");
	for (const auto& section : m_lastSections) fprintf(file, " %s=%.1f", section.name, section.ms);
	fprintf(file, "\n");
	fclose(file);
}

void FrameProfiler::SetHighPriority(bool isHigh)
{
	m_isHighPriority = isHigh;
	SetThreadPriority(GetCurrentThread(), isHigh ? THREAD_PRIORITY_HIGHEST : THREAD_PRIORITY_NORMAL);
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
	ImGui::Text("FPS %.1f (1s avg) / Period %.2f ms", m_fps, m_lastPeriodMs);
	DrawLoadBar();
	DrawMemoryBar();
	ImGui::Text("Refresh miss %d / Period jitter(>%.0fms) %d", m_refreshMissCount, kPeriodJitterMs, m_jitterCount);
	bool isHigh = m_isHighPriority;
	if (ImGui::Checkbox("High thread priority", &isHigh)) SetHighPriority(isHigh);
	//外に出したウィンドウはゲームをカクつかせる(imguiApp::Init)
	ImGuiIO& io = ImGui::GetIO();
	bool isOutside = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
	if (ImGui::Checkbox("ImGui windows outside (stutters)", &isOutside))
	{
		if (isOutside) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		else io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	}
	ImGui::PlotLines("##history", m_history.data(), kHistoryNum, m_historyIndex, nullptr, 0.0f, 40.0f, ImVec2(0, 60));
	for (const auto& section : m_lastSections) ImGui::Text("%-14s %5.2f ms", section.name, section.ms);
	ImGui::End();
}

void FrameProfiler::DrawLoadBar()
{
	const ImU32 updateColor = IM_COL32(80, 160, 255, 255);
	const ImU32 drawColor = IM_COL32(110, 210, 110, 255);
	const ImU32 otherColor = IM_COL32(190, 130, 255, 255);
	const float labelWidth = ImGui::GetFontSize() * 3.0f;

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("CPU");
	ImGui::SameLine(labelWidth);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetFrameHeight();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), ImGui::GetColorU32(ImGuiCol_FrameBg));

	//「Wait」より前(待ち以外)の区切りを、バーの端 = 1フレーム分 で並べる
	float updateMs = 0.0f;
	float drawMs = 0.0f;
	float otherMs = 0.0f;
	float x = pos.x;
	for (const auto& section : m_lastSections)
	{
		if (std::strcmp(section.name, "Wait") == 0) break;
		ImU32 color = otherColor;
		if (std::strncmp(section.name, "Upd", 3) == 0)
		{
			color = updateColor;
			updateMs += section.ms;
		}
		else if (std::strncmp(section.name, "Draw", 4) == 0)
		{
			color = drawColor;
			drawMs += section.ms;
		}
		else
		{
			otherMs += section.ms;
		}
		const float right = (std::min)(x + width * section.ms / m_expectedPeriodMs, pos.x + width);
		drawList->AddRectFilled(ImVec2(x, pos.y), ImVec2(right, pos.y + height), color);
		x = right;
	}
	const float workMs = updateMs + drawMs + otherMs;
	char text[32];
	snprintf(text, sizeof(text), "%.1f / %.1f ms", workMs, m_expectedPeriodMs);
	const ImVec2 textSize = ImGui::CalcTextSize(text);
	//1フレームに収まらないときは赤
	const ImU32 textColor = workMs > m_expectedPeriodMs ? IM_COL32(255, 80, 80, 255) : IM_COL32(255, 255, 255, 255);
	drawList->AddText(ImVec2(pos.x + (width - textSize.x) * 0.5f, pos.y + (height - textSize.y) * 0.5f), textColor, text);
	ImGui::Dummy(ImVec2(width, height));

	ImGui::SetCursorPosX(labelWidth);
	ImGui::TextColored(ImColor(updateColor), "Update %.1f", updateMs);
	ImGui::SameLine();
	ImGui::TextColored(ImColor(drawColor), "Draw %.1f", drawMs);
	ImGui::SameLine();
	ImGui::TextColored(ImColor(otherColor), "ImGui %.1f", otherMs);
}

void FrameProfiler::DrawMemoryBar()
{
	//VS の「プロセス メモリ」と同じ値(プライベート)
	PROCESS_MEMORY_COUNTERS_EX counters = {};
	counters.cb = sizeof(counters);
	if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) return;
	const float gb = static_cast<float>(counters.PrivateUsage) / kBytesPerGB;
	const float peakGb = static_cast<float>(counters.PeakPagefileUsage) / kBytesPerGB;

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Mem");
	ImGui::SameLine(ImGui::GetFontSize() * 3.0f);
	char text[48];
	snprintf(text, sizeof(text), "%.2f GB (peak %.2f GB)", gb, peakGb);
	ImGui::ProgressBar(gb / kMemoryBarMaxGB, ImVec2(-1.0f, 0.0f), text);
}
