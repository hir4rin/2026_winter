#include "PauseScene.h"
#include "SceneController.h"
#include "DxLib.h"
#include "../Input.h"
#include "../Game.h"
#include "../System.h"
#include "../BattleManager.h"
#include "StageSelectScene.h"
#include <iterator>

namespace
{
	constexpr int kDarkenAlpha = 164;//画面を少し黒くするアルファ値

	constexpr int kTextX = 300;//ポーズ中の表示のX座標
	constexpr int kTextY = 300;//ポーズ中の表示のY座標
	constexpr int kTextLineHeight = 40;//表示の行間
}

PauseScene::PauseScene(SceneController& controller) :Scene(controller)
{
	m_updateFunc = static_cast<UpdateFunc_t>(&PauseScene::NormalUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&PauseScene::NormalDraw);
}

PauseScene::~PauseScene()
{
}

void PauseScene::Update()
{
	(this->*m_updateFunc)();
}

void PauseScene::FadeInUpdate()
{
}

void PauseScene::NormalUpdate()
{
	//下のSceneMainはUpdateされないので、ここで入力を更新する
	auto& input = Input::GetInstance();
	input.Update();

	const int itemNum = static_cast<int>(MenuItem::Num);

	//上下で項目を選ぶ(端まで行ったら反対側に回る)
	if (input.IsTriggered("Up"))
	{
		m_cursor = (m_cursor + itemNum - 1) % itemNum;
	}
	if (input.IsTriggered("Down"))
	{
		m_cursor = (m_cursor + 1) % itemNum;
	}

	//Aで決定
	if (input.IsTriggered("A"))
	{
		Decide();
		return;
	}

	//Startはいつでもゲームに戻る
	if (input.IsTriggered("Start"))
	{
		m_cursor = static_cast<int>(MenuItem::ReturnGame);
		Decide();
		return;
	}
}

void PauseScene::Decide()
{
	auto battleMgr = System::GetInstance().GetBattleMgr();

	switch (static_cast<MenuItem>(m_cursor))
	{
	case MenuItem::ReturnGame:
		//フォトモード・ステージ編集を解除してSceneMainに戻る
		battleMgr->SetPhotoMode(false);
		battleMgr->SetStageEditMode(false);
		break;
	case MenuItem::PhotoMode:
		battleMgr->SetPhotoMode(true);
		battleMgr->SetStageEditMode(false);
		break;
	case MenuItem::StageEdit:
		battleMgr->SetPhotoMode(false);
		battleMgr->SetStageEditMode(true);
		break;
	case MenuItem::StageSelect:
		battleMgr->SetPhotoMode(false);
		battleMgr->SetStageEditMode(false);
		//下のSceneMainごと破棄してステージセレクトに戻る(このシーンも破棄されるので、呼んだ後は何もしない)
		m_controller.ResetScene<StageSelectScene>();
		return;
	default:
		return;
	}
	m_controller.PopScene();
}

void PauseScene::FadeOutUpdate()
{
}

void PauseScene::Draw()
{
	(this->*m_drawFunc)();
}

void PauseScene::FadeInDraw()
{
}

void PauseScene::NormalDraw()
{
	//画面を少し黒くする(下に積まれたSceneMainはSceneController::Drawで先に描画される)
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, kDarkenAlpha);
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const unsigned int white = GetColor(255, 255, 255);
	const unsigned int yellow = GetColor(255, 255, 0);
	DrawFormatString(kTextX, kTextY, white, "PauseScene  (上下:選択 A:決定 Start:ゲームに戻る)");

	const char* const itemNames[] =
	{
		"ゲームに戻る",
		"フォトモード(カメラデバッグ)",
		"ステージ編集(壁キック/壁走りゾーン)",
		"ステージセレクトへ戻る",
	};
	static_assert(std::size(itemNames) == static_cast<size_t>(MenuItem::Num), "項目名の数をMenuItemと合わせる");

	for (int i = 0; i < static_cast<int>(MenuItem::Num); ++i)
	{
		const bool isSelected = (i == m_cursor);
		DrawFormatString(kTextX, kTextY + kTextLineHeight * (i + 1), isSelected ? yellow : white,
			"%s %s", isSelected ? ">" : " ", itemNames[i]);
	}
}

void PauseScene::FadeOutDraw()
{
}
