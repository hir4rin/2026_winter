#include "ResultScene.h"
#include "SceneController.h"
#include "DxLib.h"
#include "StageSelectScene.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"

namespace
{
	constexpr int kFadeFrame = 30;//フェードにかけるフレーム数

	constexpr int kTitleX = 300;//見出しの位置
	constexpr int kTitleY = 200;
	constexpr int kGuideX = 320;//操作説明の位置
	constexpr int kGuideY = 280;
}

ResultScene::ResultScene(SceneController& controller) :Scene(controller)
{
	//スローのまま来ても戻す
	System::GetInstance().SetTimeScale(1.0f);
	Input::GetInstance().SetInputBlocked(false);

	m_updateFunc = static_cast<UpdateFunc_t>(&ResultScene::FadeInUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&ResultScene::FadeInDraw);
}

ResultScene::~ResultScene()
{
}

void ResultScene::Update()
{
	(this->*m_updateFunc)();
}

void ResultScene::FadeInUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&ResultScene::NormalUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&ResultScene::NormalDraw);
	}
}

void ResultScene::NormalUpdate()
{
	auto& input = Input::GetInstance();
	input.Update();

	//Aでステージセレクトへ
	if (input.IsTriggered("A"))
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&ResultScene::FadeOutUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&ResultScene::FadeOutDraw);
	}
}

void ResultScene::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		//このシーンはここで破棄されるので、呼んだ後は何もしない
		m_controller.ResetScene<StageSelectScene>();
		return;
	}
}

void ResultScene::Draw()
{
	(this->*m_drawFunc)();
}

void ResultScene::FadeInDraw()
{
	NormalDraw();
	DrawFade();
}

void ResultScene::NormalDraw()
{
	const unsigned int white = GetColor(255, 255, 255);

	DrawFormatString(kTitleX, kTitleY, white, "Result  STAGE CLEAR");
	DrawFormatString(kGuideX, kGuideY, white, "A:ステージセレクトへ");
}

void ResultScene::FadeOutDraw()
{
	NormalDraw();
	DrawFade();
}

void ResultScene::DrawFade()
{
	//フェードイン中は黒→透明、フェードアウト中は透明→黒
	float rate = static_cast<float>(m_fadeFrame) / static_cast<float>(kFadeFrame);
	if (m_updateFunc == static_cast<UpdateFunc_t>(&ResultScene::FadeInUpdate))
	{
		rate = 1.0f - rate;
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
