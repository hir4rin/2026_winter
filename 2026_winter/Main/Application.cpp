#include "Application.h"
#include "EffekseerForDXLib.h"
#include "Game.h"
#include "Scene/TitleScene.h"
#include "Scene/ResultScene.h"
#include "Scene/SceneMain.h"
#include <algorithm>
#include <cstring>
#include "Scene/SceneController.h"
#include "System.h"
#include "imguiApp.h"
#include "FrameProfiler.h"
#include "Render/ModelShaderManager.h"
#include "Render/CharacterLook.h"
#include <d3d11.h>
#include <dxgi.h>
#include <dwmapi.h>
#include <cstdlib>
#include <cmath>

#pragma comment(lib, "dwmapi")//DwmFlushを使うため

namespace
{
	constexpr int kEffekseerMaxParticleNum = 8000;//画面に表示する最大パーティクル数
	constexpr LONGLONG kFrameTimeMicroSec = 16667;//fpsを60に固定するための1フレームの時間(マイクロ秒)
	constexpr int kBenchFrame = 660;//-bench のときに動かすフレーム数(11秒)
	constexpr int kVSyncPacingMinHz = 58;//この範囲のリフレッシュレートなら VSync で60fpsに合わせる
	constexpr int kVSyncPacingMaxHz = 62;

}


Application::Application()
{
}

Application::~Application()
{
}

bool Application::Init()
{
	//ウィンドウモード設定
	ChangeWindowMode(true);
	//タイトル変更
	SetMainWindowText("-忍-");

	//画面のサイズ変更
	SetGraphMode(Game::kScreenWidth, Game::kScreenHeight, Game::kColorBitNum);
	//DirectInputのジョイパッド列挙で約20秒止まるため、DirectInputを使わない(XInputのコントローラーは使える)
	SetUseDirectInputFlag(false);
	//ウィンドウの外に出したImGuiを触るとゲームのウィンドウが非アクティブになるので、非アクティブでも止めない
	SetAlwaysRunFlag(TRUE);
	//画像の元データをメモリに残さない(デバイスロスト時の復元用。残すとテクスチャの分だけメモリが約0.8GB増える)
	SetUseGraphBaseDataBackup(FALSE);

	// DirectX11を使用するようにする。(DirectX9も可、一部機能不可)
	// Effekseerを使用するには必ず設定する。
	SetUseDirect3DVersion(DX_DIRECT3D_11);
	if (DxLib_Init() == -1)		// ＤＸライブラリ初期化処理
	{
		return false;			// エラーが起きたら直ちに終了
	}
	//60fps の合わせ方を決める
	InitFramePacing();
	FrameProfiler::GetInstance().SetExpectedPeriod(1000.0f / (std::max)(GetRefreshRate(), 1));
	if (strstr(GetCommandLineA(), "-prio=high")) FrameProfiler::GetInstance().SetHighPriority(true);
	//乱数の種を起動時の時刻にする(GetRandが起動ごとに違う並びになる)
	SRand(GetNowCount());
	// Effekseerを初期化する。
	// 引数には画面に表示する最大パーティクル数を設定する。
	if (Effekseer_Init(kEffekseerMaxParticleNum) == -1)
	{
		DxLib_End();
		return false;
	}
	// フルスクリーンウインドウの切り替えでリソースが消えるのを防ぐ。
	// Effekseerを使用する場合は必ず設定する。
	SetChangeScreenModeGraphicsSystemResetFlag(FALSE);



	
	//描画対象をバックバッファに変更
	SetDrawScreen(DX_SCREEN_BACK);
	// カリングの設定
	SetUseBackCulling(true);


	// Zバッファへの書き込みを有効にする。
	// Effekseerを使用する場合、2DゲームでもZバッファを使用する。
	SetWriteZBuffer3D(TRUE);

	// Zバッファを有効にする。
	// Effekseerを使用する場合、2DゲームでもZバッファを使用する。
	SetUseZBuffer3D(TRUE);

	//ImGuiの初期化(DxLibが作ったウィンドウ・DirectX11デバイスを借りて使う)
	imguiApp::GetInstance().Init(
		GetMainWindowHandle(),
		const_cast<ID3D11Device*>(static_cast<const ID3D11Device*>(GetUseDirect3D11Device())),
		const_cast<ID3D11DeviceContext*>(static_cast<const ID3D11DeviceContext*>(GetUseDirect3D11DeviceContext())));

	//どのステージでも使うもの(プレイヤー・エフェクト)の読み込み。ステージの分は入るときに読む(System::LoadStage)
	System::GetInstance().LoadCommon();
	while (GetASyncLoadNum() > 0)
	{
		if (ProcessMessage() == -1)
		{
			return false;
		}
	}
	//モデル用のシェーダーは起動時に1回だけコンパイルする(シーンをまたいで使う)
	ModelShaderManager::GetInstance().Init();

	return true;
}

void Application::Run()
{
	SceneController sceneController;
	//起動の引数に -bench を付けると、ステージ1にそのまま入って kBenchFrame フレーム動かして終わる(重さを測る用。Log/FrameStats.txt)
	//-bench=3600 のようにフレーム数も変えられる。-stage=2 でステージの番号(StageType)を選べる
	const char* benchArg = strstr(GetCommandLineA(), "-bench");
	const bool isBench = benchArg != nullptr;
	const int benchFrame = (isBench && benchArg[6] == '=') ? atoi(benchArg + 7) : kBenchFrame;
	//-result でリザルト画面だけを仮の結果で開く(見た目の確認用。自己ベストは保存しない)
	if (strstr(GetCommandLineA(), "-result"))
	{
		sceneController.ResetScene<ResultScene>(StageType::Cyber, ResultScene::MakeDebugData(), false);
	}
	else if (isBench)
	{
		const char* stageArg = strstr(GetCommandLineA(), "-stage=");
		const int stageNo = stageArg ? atoi(stageArg + 7) : static_cast<int>(StageType::Cyber);
		const bool isValidStage = stageNo >= 0 && stageNo < static_cast<int>(StageType::Num);
		sceneController.ResetScene<SceneMain>(isValidStage ? static_cast<StageType>(stageNo) : StageType::Cyber);
	}
	else
	{
		sceneController.ResetScene<TitleScene>();
	}
	int frameCount = 0;

	while (ProcessMessage() != -1)
	{
		if (isBench && ++frameCount > benchFrame) break;
		//このフレームの開始時間を取得
		LONGLONG start = GetNowHiPerformanceCount();
		FrameProfiler::GetInstance().BeginFrame();
		FrameProfiler::GetInstance().SetFrameTimeScale(System::GetInstance().GetTimeScale());

		//前のフレームに描画した内容をクリアする
		ClearDrawScreen();

		//ImGuiの新しいフレームを開始する(この後でないとImGui::Beginは使えない)
		imguiApp::GetInstance().NewFrame();

		//F2キーでImGuiのデバッグウィンドウの表示/非表示を切り替える
		if (CheckHitKey(KEY_INPUT_F2) && !m_wasF2Pressed)
		{
			imguiApp::GetInstance().ToggleDebugUIVisible();
		}
		m_wasF2Pressed = CheckHitKey(KEY_INPUT_F2) != 0;

		//ここにゲームの処理を書く
		sceneController.Update();
		FrameProfiler::GetInstance().Mark("Update");

		sceneController.Draw();
		FrameProfiler::GetInstance().Mark("Draw(rest)");
		if (imguiApp::GetInstance().IsDebugUIVisible())FrameProfiler::GetInstance().DrawDebugWindow();

		//escキーを押すとゲームを強制終了
		if (CheckHitKey(KEY_INPUT_ESCAPE))
		{
			break;
		}
		//組み立てたImGuiのウィンドウを描画する(ScreenFlipより前に呼ぶ)
		imguiApp::GetInstance().Render();
		FrameProfiler::GetInstance().Mark("ImGui");
		//描画した内容をすぐ画面に出す
		//待ってから出すと絵が1フレーム古くなる
		FrameProfiler::GetInstance().MarkGpuFrameEnd();
		ScreenFlip();
		FrameProfiler::GetInstance().Mark("Flip");
		//外に出したImGuiのウィンドウはゲームの画面を出した後に描く
		imguiApp::GetInstance().RenderPlatformWindows();
		FrameProfiler::GetInstance().Mark("ImGui(out)");
		//出した後にモニターの更新を待つ
		//次のフレームは更新の直後から始まる
		bool isWaited = m_paceMode == PaceMode::None;
		if (m_paceMode == PaceMode::WaitVSync)
		{
			WaitVSync(1);
			isWaited = true;
		}
		else if (m_paceMode == PaceMode::VBlank)
		{
			isWaited = WaitVBlank();
			if (!isWaited)
			{
				WaitVSync(1);//取れなかったときは DxLib の待ち
				isWaited = true;
			}
		}
		//画面の合成を待つ(フルスクリーンなどで使えないときは自前の待ちにする)
		if (m_paceMode == PaceMode::Dwm && !m_isFullScreen)
		{
			//最小化中などはすぐ返ってくるので、短すぎたら自前の待ちもする
			isWaited = SUCCEEDED(DwmFlush()) && GetNowHiPerformanceCount() - start >= kFrameTimeMicroSec / 2;
		}
		//待ち時間の記録
		FrameProfiler::GetInstance().Mark("Wait");
		const bool isVBlankWaited = m_paceMode == PaceMode::WaitVSync || m_paceMode == PaceMode::VBlank;
		FrameProfiler::GetInstance().UpdateLatency(isVBlankWaited);
		const long long refreshIndex = GetRefreshIndex();
		if (refreshIndex >= 0) FrameProfiler::GetInstance().SetRefreshIndex(refreshIndex);
		FrameProfiler::GetInstance().EndFrame();

		//フレームレート60に固定(上で待ったときは何もしない)
		while (!isWaited && GetNowHiPerformanceCount() - start < kFrameTimeMicroSec)
		{

		}
	}
}

void Application::Terminate()
{
	FrameProfiler::GetInstance().Terminate();
	ReleaseVBlankOutput();
	//ImGuiのリソースを解放する(DxLib_Endより前)
	imguiApp::GetInstance().Uninit();

	//読み込んだモデル・エフェクトを解放する(Effekseer・DxLibの終了より前に行う)
	System::GetInstance().Terminate();
	//シーンをまたいで使い回しているシェーダー・キャラの画像
	ModelShaderManager::GetInstance().Terminate();
	CharacterLook::ReleaseGraphCache();

	// Effekseerを終了する。
	Effkseer_End();

	DxLib_End();
}

void Application::InitFramePacing()
{
	//モニターが60Hz前後なら、画面に出した後にモニターの更新(WaitForVBlank)を待つ
	//それ以外(120/144Hz など)なら Run の自前の待ちで合わせる
	//(このPCでは ScreenFlip の VSync 待ちが効かず、自前の待ちだけだと少しずつずれて引っかかる)
	const int refreshRate = GetRefreshRate();
	m_isVSyncPacing = refreshRate >= kVSyncPacingMinHz && refreshRate <= kVSyncPacingMaxHz;
	//(確認用)-pace=busy / both / wait / dwm / vblank で待ち方を切り替える
	const char* commandLine = GetCommandLineA();
	if (strstr(commandLine, "-pace=busy")) m_paceMode = PaceMode::Busy;
	else if (strstr(commandLine, "-pace=both")) m_paceMode = PaceMode::VSyncAndBusy;
	else if (strstr(commandLine, "-pace=wait")) m_paceMode = PaceMode::WaitVSync;
	else if (strstr(commandLine, "-pace=dwm")) m_paceMode = PaceMode::Dwm;
	else if (strstr(commandLine, "-pace=vblank")) m_paceMode = PaceMode::VBlank;
	else if (strstr(commandLine, "-pace=none")) m_paceMode = PaceMode::None;
	else m_paceMode = m_isVSyncPacing ? PaceMode::VBlank : PaceMode::Busy;
	//ScreenFlip の VSync 待ちは前のやり方(both)のときだけ
	SetWaitVSyncFlag(m_paceMode == PaceMode::VSyncAndBusy ? TRUE : FALSE);
}

bool Application::WaitVBlank()
{
	//ウィンドウが別のモニターに移ったら取り直す
	if (MonitorFromWindow(GetMainWindowHandle(), MONITOR_DEFAULTTONEAREST) != m_vblankMonitor)
	{
		UpdateVBlankOutput();
	}
	if (!m_vblankOutput) return false;
	return SUCCEEDED(m_vblankOutput->WaitForVBlank());
}

void Application::UpdateVBlankOutput()
{
	ReleaseVBlankOutput();
	HMONITOR monitor = MonitorFromWindow(GetMainWindowHandle(), MONITOR_DEFAULTTONEAREST);
	m_vblankMonitor = monitor;
	auto* device = const_cast<ID3D11Device*>(static_cast<const ID3D11Device*>(GetUseDirect3D11Device()));
	if (!device) return;
	IDXGIDevice* dxgiDevice = nullptr;
	if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice)))) return;
	IDXGIAdapter* adapter = nullptr;
	if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter)))
	{
		//アダプターの出力からウィンドウがあるモニターを探す
		IDXGIOutput* output = nullptr;
		for (UINT i = 0; adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_OUTPUT_DESC desc = {};
			if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == monitor)
			{
				m_vblankOutput = output;
				break;
			}
			output->Release();
		}
		adapter->Release();
	}
	dxgiDevice->Release();
}

long long Application::GetRefreshIndex()
{
	DWM_TIMING_INFO info = {};
	info.cbSize = sizeof(info);
	if (FAILED(DwmGetCompositionTimingInfo(nullptr, &info)) || info.qpcRefreshPeriod == 0) return -1;
	//前に呼んでからの時間 ÷ 更新1回分の時間 で数える
	//(cRefresh は画面の合成の回数なので、モニターが2台点いていると1回の更新で2つ進む)
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	if (m_lastRefreshQpc >= 0)
	{
		m_refreshIndex += std::llround(static_cast<double>(now.QuadPart - m_lastRefreshQpc) / static_cast<double>(info.qpcRefreshPeriod));
	}
	m_lastRefreshQpc = now.QuadPart;
	return m_refreshIndex;
}

void Application::ReleaseVBlankOutput()
{
	if (m_vblankOutput)
	{
		m_vblankOutput->Release();
		m_vblankOutput = nullptr;
	}
}