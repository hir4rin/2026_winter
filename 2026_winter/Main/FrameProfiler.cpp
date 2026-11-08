#include "FrameProfiler.h"
#include "imgui.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <psapi.h>
#include <d3d11.h>
#include <dwmapi.h>

namespace
{
	const char* const kSpikeLogPath = "Log/FrameSpikes.txt";
	const char* const kStatLogPath = "Log/FrameStats.txt";
	const char* const kEventLogPath = "Log/CameraEvents.txt";
	const char* const kTimeScaleLogPath = "Log/TimeScale.txt";

	//フルパスからファイル名だけ
	const char* FileNameOf(const char* path)
	{
		const char* name = path;
		for (const char* p = path; *p != '\0'; ++p)
		{
			if (*p == '\\' || *p == '/') name = p + 1;
		}
		return name;
	}
	constexpr float kBytesPerGB = 1024.0f * 1024.0f * 1024.0f;
}

FrameProfiler::~FrameProfiler()
{
	Terminate();
}

void FrameProfiler::Terminate()
{
	ReleaseGpuQueries();
	{
		std::lock_guard<std::mutex> lock(m_logMutex);
		m_isLogStop = true;
	}
	m_logWake.notify_one();
	if (m_logThread.joinable()) m_logThread.join();
}

void FrameProfiler::LogEvent(const char* format, ...)
{
	AddLog(LogFile::Event, "frame %d ", m_frameCount);
	va_list args;
	va_start(args, format);
	AddLogV(LogFile::Event, format, args);
	va_end(args);
	AddLog(LogFile::Event, "\n");
}

void FrameProfiler::AddLog(LogFile file, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	AddLogV(file, format, args);
	va_end(args);
}

void FrameProfiler::AddLogV(LogFile file, const char* format, va_list args)
{
	char text[kLogTextMax];
	const int length = vsnprintf(text, sizeof(text), format, args);
	if (length <= 0) return;
	std::lock_guard<std::mutex> lock(m_logMutex);
	if (m_isLogStop) return;
	m_logBuffers[static_cast<int>(file)].append(text, (std::min)(length, kLogTextMax - 1));
	if (!m_logThread.joinable())
	{
		m_logThread = std::thread(&FrameProfiler::LogThread, this);
		SetThreadPriority(m_logThread.native_handle(), THREAD_PRIORITY_BELOW_NORMAL);
	}
}

void FrameProfiler::AddSectionLog()
{
	for (const auto& section : m_lastSections) AddLog(LogFile::Spike, " %s=%.1f", section.name, section.ms);
	AddLog(LogFile::Spike, "\n");
}

void FrameProfiler::LogThread()
{
	bool isStop = false;
	while (!isStop)
	{
		{
			std::unique_lock<std::mutex> lock(m_logMutex);
			m_logWake.wait_for(lock, std::chrono::milliseconds(kLogIntervalMs), [this] { return m_isLogStop; });
			m_logBuffers.swap(m_logWriting);
			isStop = m_isLogStop;
		}
		WriteLogFiles();
	}
}

void FrameProfiler::WriteLogFiles()
{
	const char* const paths[kLogFileNum] = { kSpikeLogPath, kStatLogPath, kEventLogPath, kTimeScaleLogPath };
	for (int i = 0; i < kLogFileNum; ++i)
	{
		std::string& text = m_logWriting[i];
		if (text.empty()) continue;
		//起動して最初の記録のときにファイルを作り直す
		if (!m_isLogFileStarted[i]) std::filesystem::create_directories("Log");
		FILE* file = nullptr;
		if (fopen_s(&file, paths[i], m_isLogFileStarted[i] ? "a" : "w") == 0 && file != nullptr)
		{
			m_isLogFileStarted[i] = true;
			fwrite(text.data(), 1, text.size(), file);
			fclose(file);
		}
		text.clear();
	}
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

		//前のフレームの、画面で見た速さ(タイムスケール × 本来の間隔 ÷ 実際の間隔)
		//フレームが落ちると、タイムスケールが1でも遅く見える
		if (periodMs >= m_expectedPeriodMs * kDropRate) ++m_dropCount;
		m_scaleHistory[m_speedHistoryIndex] = m_frameTimeScale;
		m_speedHistory[m_speedHistoryIndex] = periodMs > 0.0f ? m_frameTimeScale * m_expectedPeriodMs / periodMs : 0.0f;
		m_speedHistoryIndex = (m_speedHistoryIndex + 1) % kHistoryNum;

		//一番間隔が長かったフレームの内訳を残す(m_lastSections はまだ前のフレーム)
		++m_worstAge;
		if (periodMs >= m_worstPeriodMs || m_worstAge >= kHistoryNum)
		{
			m_worstPeriodMs = periodMs;
			m_worstFrame = m_frameCount - 1;
			m_worstSections = m_lastSections;
			m_worstAge = 0;
		}
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
	AddLog(LogFile::Stat, "frame %d (avg/min/max ms):", m_frameCount);
	for (const auto& stat : m_stats) AddLog(LogFile::Stat, " %s=%.2f/%.1f/%.1f", stat.name, stat.sum / m_statCount, stat.min, stat.max);
	AddLog(LogFile::Stat, "\n");
	m_stats.clear();
	m_statCount = 0;
}

void FrameProfiler::WriteSpike(float totalMs)
{
	++m_spikeCount;
	AddLog(LogFile::Spike, "frame %d total %.1fms:", m_frameCount, totalMs);
	AddSectionLog();
}

void FrameProfiler::WritePeriodJitter(float periodMs)
{
	++m_jitterCount;
	//m_lastSections はまだ前のフレームの内訳
	AddLog(LogFile::Spike, "frame %d period %.1fms%s:", m_frameCount - 1, periodMs, m_isHighPriority ? " (high)" : "");
	AddSectionLog();
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
			AddLog(LogFile::Spike, "frame %d refresh +%lld (1 が正常)\n", m_frameCount, delta);
		}
	}
	m_lastRefreshIndex = refreshIndex;
}

void FrameProfiler::RecordTimeScale(float from, float to, const char* file, int line)
{
	const char* fileName = FileNameOf(file);
	m_timeScaleChanges[m_timeScaleChangeIndex] = { m_frameCount, from, to, fileName, line };
	m_timeScaleChangeIndex = (m_timeScaleChangeIndex + 1) % kTimeScaleChangeNum;
	++m_timeScaleChangeTotal;
	AddLog(LogFile::TimeScale, "frame %d %.3f -> %.3f %s:%d\n", m_frameCount, from, to, fileName, line);
}

void FrameProfiler::DrawSpeedGraph()
{
	const ImU32 scaleColor = IM_COL32(255, 210, 80, 255);
	const ImU32 speedColor = IM_COL32(80, 220, 255, 255);
	const int latest = (m_speedHistoryIndex + kHistoryNum - 1) % kHistoryNum;

	ImGui::TextColored(ImColor(speedColor), "Speed %.2f", m_speedHistory[latest]);
	ImGui::SameLine();
	ImGui::TextColored(ImColor(scaleColor), "TimeScale %.2f", m_scaleHistory[latest]);
	ImGui::SameLine();
	ImGui::Text("Drop(>=%.1fx) %d", kDropRate, m_dropCount);

	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetFrameHeight() * 2.5f;
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), ImGui::GetColorU32(ImGuiCol_FrameBg));
	auto toY = [&](float value) { return pos.y + height * (1.0f - (std::clamp)(value, 0.0f, kSpeedGraphMax) / kSpeedGraphMax); };
	//1.0(普通の速さ)と0.5の線
	drawList->AddLine(ImVec2(pos.x, toY(1.0f)), ImVec2(pos.x + width, toY(1.0f)), IM_COL32(255, 255, 255, 90));
	drawList->AddLine(ImVec2(pos.x, toY(0.5f)), ImVec2(pos.x + width, toY(0.5f)), IM_COL32(255, 255, 255, 40));
	//古い順に左から
	auto drawLine = [&](const std::array<float, kHistoryNum>& values, ImU32 color)
		{
			for (int i = 1; i < kHistoryNum; ++i)
			{
				const int prev = (m_speedHistoryIndex + i - 1) % kHistoryNum;
				const int cur = (m_speedHistoryIndex + i) % kHistoryNum;
				const float x0 = pos.x + width * (i - 1) / (kHistoryNum - 1);
				const float x1 = pos.x + width * i / (kHistoryNum - 1);
				drawList->AddLine(ImVec2(x0, toY(values[prev])), ImVec2(x1, toY(values[cur])), color, 1.5f);
			}
		};
	drawLine(m_scaleHistory, scaleColor);
	drawLine(m_speedHistory, speedColor);
	ImGui::Dummy(ImVec2(width, height));
}

void FrameProfiler::DrawWorstFrame()
{
	if (m_worstFrame < 0) return;
	const bool isDrop = m_worstPeriodMs >= m_expectedPeriodMs * kDropRate;
	const ImVec4 titleColor = isDrop ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	ImGui::TextColored(titleColor, "Worst period(3s) %.1f ms (frame %d)", m_worstPeriodMs, m_worstFrame);
	//待ち以外で一番長い区切りを赤くする
	float longestMs = 0.0f;
	for (const auto& section : m_worstSections)
	{
		if (std::strcmp(section.name, "Wait") == 0) continue;
		longestMs = (std::max)(longestMs, section.ms);
	}
	for (const auto& section : m_worstSections)
	{
		const bool isLongest = std::strcmp(section.name, "Wait") != 0 && section.ms == longestMs && longestMs > 0.0f;
		if (isLongest) ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "  %-14s %5.2f ms", section.name, section.ms);
		else ImGui::Text("  %-14s %5.2f ms", section.name, section.ms);
	}
}

void FrameProfiler::DrawTimeScaleChanges()
{
	ImGui::Text("TimeScale changes %d (Log/TimeScale.txt)", m_timeScaleChangeTotal);
	//新しい順
	const int num = (std::min)(m_timeScaleChangeTotal, kTimeScaleChangeNum);
	for (int i = 0; i < num; ++i)
	{
		const auto& change = m_timeScaleChanges[(m_timeScaleChangeIndex - 1 - i + kTimeScaleChangeNum) % kTimeScaleChangeNum];
		ImGui::Text("  f%-6d %.2f -> %.2f  %s:%d", change.frame, change.from, change.to, change.file, change.line);
	}
}

void FrameProfiler::MarkGpuFrameEnd()
{
	auto* device = static_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
	auto* context = static_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));
	if (!device || !context) return;
	ID3D11Query*& query = m_gpuQueries[m_gpuQueryIndex];
	if (!query)
	{
		D3D11_QUERY_DESC desc = {};
		desc.Query = D3D11_QUERY_EVENT;
		if (FAILED(device->CreateQuery(&desc, &query)))
		{
			query = nullptr;
			return;
		}
	}
	//ここまでの描画が GPU で終わると、この印が終わった扱いになる
	context->End(query);
	m_isGpuQueryIssued[m_gpuQueryIndex] = true;
	m_gpuQueryIndex = (m_gpuQueryIndex + 1) % kGpuQueryNum;
}

int FrameProfiler::CountGpuBehind()
{
	auto* context = static_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));
	if (!context) return 0;
	int count = 0;
	for (int i = 0; i < kGpuQueryNum; ++i)
	{
		if (!m_isGpuQueryIssued[i]) continue;
		//待たずに終わったかだけ見る
		if (context->GetData(m_gpuQueries[i], nullptr, 0, D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK)
		{
			m_isGpuQueryIssued[i] = false;
		}
		else
		{
			++count;
		}
	}
	return count;
}

void FrameProfiler::ReleaseGpuQueries()
{
	for (int i = 0; i < kGpuQueryNum; ++i)
	{
		if (m_gpuQueries[i]) m_gpuQueries[i]->Release();
		m_gpuQueries[i] = nullptr;
		m_isGpuQueryIssued[i] = false;
	}
}

void FrameProfiler::UpdateLatency(bool isVBlankWaited)
{
	const float periodMs = m_expectedPeriodMs;
	const float nowMs = static_cast<float>(GetNowHiPerformanceCount() - m_frameStart) / 1000.0f;
	//フレームの始まりから ScreenFlip が返るまで
	float flipMs = 0.0f;
	bool isFlipFound = false;
	for (const auto& section : m_sections)
	{
		flipMs += section.ms;
		if (std::strcmp(section.name, "Flip") == 0)
		{
			isFlipFound = true;
			break;
		}
	}
	if (!isFlipFound) return;

	//ScreenFlip の後の最初のモニターの更新(フレームの始まりから)
	//待ったときは今がその更新
	float vblankMs = nowMs;
	if (!isVBlankWaited)
	{
		//待っていないときは、Windows の最後の更新の時刻から数える
		DWM_TIMING_INFO info = {};
		info.cbSize = sizeof(info);
		LARGE_INTEGER qpc = {};
		LARGE_INTEGER freq = {};
		if (FAILED(DwmGetCompositionTimingInfo(nullptr, &info)) || !QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) return;
		QueryPerformanceCounter(&qpc);
		const float ageMs = static_cast<float>(static_cast<double>(qpc.QuadPart - static_cast<LONGLONG>(info.qpcVBlank)) * 1000.0 / static_cast<double>(freq.QuadPart));
		vblankMs = nowMs - ageMs;
		vblankMs += std::ceil((flipMs - vblankMs) / periodMs) * periodMs;
	}

	m_latency.flipMs = flipMs;
	m_latency.vblankMs = (std::max)(vblankMs - flipMs, 0.0f);
	//更新を待った後に GPU が終わっていなければ、その分だけ画面に出るのが遅れる
	//待っていないときは、このフレームの印はまだ終わっていなくてよい
	const int behind = CountGpuBehind();
	m_latency.gpuBehindFrame = isVBlankWaited ? behind : (std::max)(behind - 1, 0);
	m_latency.gpuLateMs = m_latency.gpuBehindFrame * periodMs;
	//ウィンドウモードは、Windows が次の更新で画面を合成して出す
	m_latency.composeMs = periodMs;

	const float totalMs = m_latency.Total();
	AddStat("LATENCY", totalMs);
	m_latencySum += totalMs;
	m_latencyMaxWork = (std::max)(m_latencyMaxWork, totalMs);
	if (++m_latencyNum >= kStatFrame)
	{
		m_latencyAvg = m_latencySum / m_latencyNum;
		m_latencyMax = m_latencyMaxWork;
		m_latencySum = 0.0f;
		m_latencyMaxWork = 0.0f;
		m_latencyNum = 0;
	}
}

void FrameProfiler::DrawLatencyBar()
{
	const ImU32 flipColor = IM_COL32(255, 170, 60, 255);
	const ImU32 vblankColor = IM_COL32(120, 150, 190, 255);
	const ImU32 gpuColor = IM_COL32(255, 80, 80, 255);
	const ImU32 composeColor = IM_COL32(110, 110, 130, 255);
	const float labelWidth = ImGui::GetFontSize() * 3.0f;

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Lag");
	ImGui::SameLine(labelWidth);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const float width = ImGui::GetContentRegionAvail().x;
	const float height = ImGui::GetFrameHeight();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), ImGui::GetColorU32(ImGuiCol_FrameBg));

	//バーの端 = kLatencyBarFrame フレーム分
	const float fullMs = m_expectedPeriodMs * kLatencyBarFrame;
	const float parts[] = { m_latency.flipMs, m_latency.vblankMs, m_latency.gpuLateMs, m_latency.composeMs };
	const ImU32 colors[] = { flipColor, vblankColor, gpuColor, composeColor };
	float x = pos.x;
	for (int i = 0; i < 4; ++i)
	{
		const float right = (std::min)(x + width * parts[i] / fullMs, pos.x + width);
		drawList->AddRectFilled(ImVec2(x, pos.y), ImVec2(right, pos.y + height), colors[i]);
		x = right;
	}
	//1フレームごとの目盛り
	for (int i = 1; i < static_cast<int>(kLatencyBarFrame); ++i)
	{
		const float lineX = pos.x + width * i / kLatencyBarFrame;
		drawList->AddLine(ImVec2(lineX, pos.y), ImVec2(lineX, pos.y + height), IM_COL32(0, 0, 0, 160));
	}
	const float totalMs = m_latency.Total();
	char text[48];
	snprintf(text, sizeof(text), "%.1f ms (%.1f frames)", totalMs, totalMs / m_expectedPeriodMs);
	const ImVec2 textSize = ImGui::CalcTextSize(text);
	//GPU が間に合っていないときは赤
	const ImU32 textColor = m_latency.gpuBehindFrame > 0 ? IM_COL32(255, 80, 80, 255) : IM_COL32(255, 255, 255, 255);
	drawList->AddText(ImVec2(pos.x + (width - textSize.x) * 0.5f, pos.y + (height - textSize.y) * 0.5f), textColor, text);
	ImGui::Dummy(ImVec2(width, height));

	ImGui::SetCursorPosX(labelWidth);
	ImGui::TextColored(ImColor(flipColor), "Flip %.1f", m_latency.flipMs);
	ImGui::SameLine();
	ImGui::TextColored(ImColor(vblankColor), "VBlank %.1f", m_latency.vblankMs);
	ImGui::SameLine();
	ImGui::TextColored(ImColor(gpuColor), "GPU late %.1f(%d)", m_latency.gpuLateMs, m_latency.gpuBehindFrame);
	ImGui::SameLine();
	ImGui::TextColored(ImColor(composeColor), "DWM %.1f", m_latency.composeMs);
	ImGui::SetCursorPosX(labelWidth);
	ImGui::Text("Avg %.1f / Max %.1f ms (%dF)", m_latencyAvg, m_latencyMax, kStatFrame);
}

void FrameProfiler::SetupWindowPlacement()
{
	ImGuiIO& io = ImGui::GetIO();
	const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
	if (!m_isWindowOutside)
	{
		//中に戻した最初だけ、ゲームの画面の左上に置く
		if (!m_isWindowPlaced)
		{
			ImGui::SetNextWindowPos(ImVec2(mainViewport->Pos.x + 20.0f, mainViewport->Pos.y + 40.0f), ImGuiCond_Always);
			m_isWindowPlaced = true;
		}
		return;
	}
	//外に出す機能を入れたフレームは、まだ外に出せないので次のフレームで置く
	const bool isViewportsEnabled = (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	//ゲームのウィンドウにくっつかず、いつも別のウィンドウにする
	ImGuiWindowClass windowClass;
	windowClass.ViewportFlagsOverrideSet = ImGuiViewportFlags_NoAutoMerge;
	ImGui::SetNextWindowClass(&windowClass);
	if (m_isWindowPlaced || !isViewportsEnabled) return;

	//外に出した最初だけ、ゲームのウィンドウの右に置く(はみ出すならモニターの中に寄せる)
	float x = mainViewport->Pos.x + mainViewport->Size.x + kOutsideMargin;
	float y = mainViewport->Pos.y;
	float height = kOutsideHeight;
	const ImGuiPlatformMonitor* monitor = nullptr;
	for (const auto& candidate : ImGui::GetPlatformIO().Monitors)
	{
		const bool isInsideX = mainViewport->Pos.x >= candidate.WorkPos.x && mainViewport->Pos.x < candidate.WorkPos.x + candidate.WorkSize.x;
		const bool isInsideY = mainViewport->Pos.y >= candidate.WorkPos.y && mainViewport->Pos.y < candidate.WorkPos.y + candidate.WorkSize.y;
		if (isInsideX && isInsideY) monitor = &candidate;
	}
	if (monitor)
	{
		x = (std::min)(x, monitor->WorkPos.x + monitor->WorkSize.x - kOutsideWidth);
		height = (std::min)(height, monitor->WorkSize.y);
		y = (std::clamp)(y, monitor->WorkPos.y, monitor->WorkPos.y + monitor->WorkSize.y - height);
	}
	ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(kOutsideWidth, height), ImGuiCond_Always);
	ImGui::SetNextWindowDockID(0, ImGuiCond_Always);//ほかのウィンドウとのドッキングを外す
	m_isWindowPlaced = true;
}

void FrameProfiler::DrawDebugWindow()
{
	SetupWindowPlacement();
	ImGui::Begin("Frame Time");
	float worst = 0.0f;
	for (float ms : m_history) worst = (std::max)(worst, ms);
	ImGui::Text("Last %.1f ms / Worst(3s) %.1f ms / Spikes(>%.0fms) %d", m_lastTotalMs, worst, kSpikeMs, m_spikeCount);
	ImGui::Text("FPS %.1f (1s avg) / Period %.2f ms", m_fps, m_lastPeriodMs);
	DrawLatencyBar();
	DrawLoadBar();
	DrawMemoryBar();
	ImGui::Text("Refresh miss %d / Period jitter(>%.0fms) %d", m_refreshMissCount, kPeriodJitterMs, m_jitterCount);
	bool isHigh = m_isHighPriority;
	if (ImGui::Checkbox("High thread priority", &isHigh)) SetHighPriority(isHigh);
	//外に出したウィンドウはゲームをカクつかせることがある(imguiApp::Init)
	//切ると、ほかに外に出したウィンドウも中に戻る
	if (ImGui::Checkbox("Outside game window (may stutter)", &m_isWindowOutside))
	{
		m_isWindowPlaced = false;
		if (!m_isWindowOutside) ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	}
	ImGui::PlotLines("##history", m_history.data(), kHistoryNum, m_historyIndex, nullptr, 0.0f, 40.0f, ImVec2(0, 60));
	if (ImGui::TreeNodeEx("Speed", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawSpeedGraph();
		DrawTimeScaleChanges();
		ImGui::TreePop();
	}
	if (ImGui::TreeNodeEx("Worst frame", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawWorstFrame();
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Last frame"))
	{
		for (const auto& section : m_lastSections) ImGui::Text("%-14s %5.2f ms", section.name, section.ms);
		ImGui::TreePop();
	}
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
