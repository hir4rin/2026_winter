#include "StageSelectScene.h"
#include "SceneController.h"
#include "DxLib.h"
#include "../SceneMain.h"
#include "../Input.h"
#include "../Game.h"
#include "../System.h"
#include "../Stage/StageInfo.h"

namespace
{
	constexpr int kFadeFrame = 30;//フェードにかけるフレーム数

	constexpr int kTitleX = 300;//見出しのX座標
	constexpr int kTitleY = 200;//見出しのY座標
	constexpr int kItemX = 320;//ステージ名のX座標
	constexpr int kItemY = 280;//最初のステージ名のY座標
	constexpr int kItemLineHeight = 60;//ステージ名の行間
	constexpr int kDescOffsetY = 24;//ステージ名から説明までの距離
}

int StageSelectScene::s_cursor = 0;

StageSelectScene::StageSelectScene(SceneController& controller) :Scene(controller)
{
	//最初のシーンなので、ここで入力を使えるようにしておく(SceneMainでもInitする)
	Input::GetInstance().Init();
	//ゲーム中にヒットストップなどで時間が止まったまま戻ってきても大丈夫なように戻しておく
	System::GetInstance().SetTimeScale(1.0f);

	m_updateFunc = static_cast<UpdateFunc_t>(&StageSelectScene::FadeInUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&StageSelectScene::FadeInDraw);
}

StageSelectScene::~StageSelectScene()
{
}

void StageSelectScene::Update()
{
	(this->*m_updateFunc)();
}

void StageSelectScene::FadeInUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&StageSelectScene::NormalUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&StageSelectScene::NormalDraw);
	}
}

void StageSelectScene::NormalUpdate()
{
	auto& input = Input::GetInstance();
	input.Update();

	const int stageNum = static_cast<int>(StageType::Num);

	//上下でステージを選ぶ(端まで行ったら反対側に回る)
	if (input.IsTriggered("Up"))
	{
		s_cursor = (s_cursor + stageNum - 1) % stageNum;
	}
	if (input.IsTriggered("Down"))
	{
		s_cursor = (s_cursor + 1) % stageNum;
	}

	//Aで決定してフェードアウト
	if (input.IsTriggered("A"))
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&StageSelectScene::FadeOutUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&StageSelectScene::FadeOutDraw);
	}
}

void StageSelectScene::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		//このシーンはここで破棄されるので、呼んだ後は何もしない
		m_controller.ResetScene<SceneMain>(static_cast<StageType>(s_cursor));
		return;
	}
}

void StageSelectScene::Draw()
{
	(this->*m_drawFunc)();
}

void StageSelectScene::FadeInDraw()
{
	NormalDraw();
	DrawFade();
}

void StageSelectScene::NormalDraw()
{
	const unsigned int white = GetColor(255, 255, 255);
	const unsigned int yellow = GetColor(255, 255, 0);
	const unsigned int gray = GetColor(160, 160, 160);

	DrawFormatString(kTitleX, kTitleY, white, "StageSelect  (上下:選択 A:決定)");

	for (int i = 0; i < static_cast<int>(StageType::Num); ++i)
	{
		const StageInfo& info = StageData::GetInfo(static_cast<StageType>(i));
		const bool isSelected = (i == s_cursor);
		const int y = kItemY + kItemLineHeight * i;
		DrawFormatString(kItemX, y, isSelected ? yellow : white, "%s %s", isSelected ? ">" : " ", info.name);
		DrawFormatString(kItemX + 20, y + kDescOffsetY, gray, "%s", info.description);
	}
}

void StageSelectScene::FadeOutDraw()
{
	NormalDraw();
	DrawFade();
}

void StageSelectScene::DrawFade()
{
	//フェードイン中は黒→透明、フェードアウト中は透明→黒
	float rate = static_cast<float>(m_fadeFrame) / static_cast<float>(kFadeFrame);
	if (m_updateFunc == static_cast<UpdateFunc_t>(&StageSelectScene::FadeInUpdate))
	{
		rate = 1.0f - rate;
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
