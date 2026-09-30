#include "Application.h"
#include "EffekseerForDXLib.h"
#include "Game.h"
#include "SceneMain.h"
#include "Scene/SceneController.h"
#include "System.h"
#include "imguiApp.h"
#include <d3d11.h>

namespace
{
	constexpr int kEffekseerMaxParticleNum = 8000;//画面に表示する最大パーティクル数
	constexpr LONGLONG kFrameTimeMicroSec = 16667;//fpsを60に固定するための1フレームの時間(マイクロ秒)

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


	// DirectX11を使用するようにする。(DirectX9も可、一部機能不可)
	// Effekseerを使用するには必ず設定する。
	SetUseDirect3DVersion(DX_DIRECT3D_11);
	if (DxLib_Init() == -1)		// ＤＸライブラリ初期化処理
	{
		return false;			// エラーが起きたら直ちに終了
	}
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

	//モデル・エフェクトの読み込み
	System::GetInstance().LoadAll();
	//TODO:ローディングシーンができたら、そちらで待つようにする
	while (GetASyncLoadNum() > 0)
	{
		if (ProcessMessage() == -1)
		{
			return false;
		}
	}

	return true;
}

void Application::Run()
{
	SceneController sceneController;
	sceneController.ResetScene<SceneMain>();

	while (ProcessMessage() != -1)
	{
		//このフレームの開始時間を取得
		LONGLONG start = GetNowHiPerformanceCount();

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

		sceneController.Draw();

		//escキーを押すとゲームを強制終了
		if (CheckHitKey(KEY_INPUT_ESCAPE))
		{
			break;
		}
		//組み立てたImGuiのウィンドウを描画する(ScreenFlipより前に呼ぶ)
		imguiApp::GetInstance().Render();
		//描画した内容を画面に反映する
		ScreenFlip();

		//フレームレート60に固定
		while (GetNowHiPerformanceCount() - start < kFrameTimeMicroSec)
		{

		}
	}
}

void Application::Terminate()
{
	//ImGuiのリソースを解放する(DxLib_Endより前)
	imguiApp::GetInstance().Uninit();

	//読み込んだモデル・エフェクトを解放する(Effekseer・DxLibの終了より前に行う)
	System::GetInstance().Terminate();

	// Effekseerを終了する。
	Effkseer_End();

	DxLib_End();
}