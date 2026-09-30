#include "PauseScene.h"
#include "SceneController.h"
#include "DxLib.h"
#include "../Input.h"
#include "../Game.h"
#include "../System.h"
#include "../BattleManager.h"

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

	auto battleMgr = System::GetInstance().GetBattleMgr();

	if (input.IsTriggered("Y"))
	{
		//フォトモードにして、ポーズを解除してSceneMainに戻る
		battleMgr->SetPhotoMode(true);
		m_controller.PopScene();
		return;
	}

	if (input.IsTriggered("Start"))
	{
		//フォトモードを解除して、ポーズを解除してSceneMainに戻る
		battleMgr->SetPhotoMode(false);
		m_controller.PopScene();
		return;
	}
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
	DrawFormatString(kTextX, kTextY, white, "PauseScene");
	DrawFormatString(kTextX, kTextY + kTextLineHeight, white, "Yボタン : フォトモード");
	DrawFormatString(kTextX, kTextY + kTextLineHeight * 2, white, "Startボタン : ゲームに戻る");
}

void PauseScene::FadeOutDraw()
{
}
