#include "DxLib.h"
#include "Game.h"

#include <memory> //シェアードポインタ

#include "SceneMain.h"

// プログラムは WinMain から始まります
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//ウィンドウモード設定
	ChangeWindowMode(true);
	//タイトル変更
	SetMainWindowText("-忍-");
	//画面のサイズ変更
	SetGraphMode(Game::kScreenWidth, Game::kScreenHeight, Game::kColorBitNum);
	if (DxLib_Init() == -1)		// ＤＸライブラリ初期化処理
	{
		return -1;			// エラーが起きたら直ちに終了
	}

	//描画対象をバックバッファに変更
	SetDrawScreen(DX_SCREEN_BACK);
	// カリングの設定
	SetUseBackCulling(true);
	//シーンの作成
	std::shared_ptr<SceneMain> pScene = std::make_shared<SceneMain>();
	pScene->Init();

	while (ProcessMessage() != -1)
	{
		//このフレームの開始時間を取得
		LONGLONG start = GetNowHiPerformanceCount();

		//前のフレームに描画した内容をクリアする
		ClearDrawScreen();

		//ここにゲームの処理を書く
		pScene->Update();

		pScene->Draw();

		//escキーを押すとゲームを強制終了
		if (CheckHitKey(KEY_INPUT_ESCAPE))
		{
			break;
		}
		//描画した内容を画面に反映する
		ScreenFlip();

		//フレームレート60に固定
		while (GetNowHiPerformanceCount() - start < 16667)
		{

		}
	}

	DxLib_End();				// ＤＸライブラリ使用の終了処理

	return 0;				// ソフトの終了
}
