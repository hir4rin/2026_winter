#include "StageSelectScene.h"
#include "SceneController.h"
#include "DxLib.h"
#include "SceneMain.h"
#include "TutorialScene.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "../Stage/StageInfo.h"

namespace
{
	constexpr int kFadeFrame = 30;//フェードにかけるフレーム数

	constexpr int kLoadingTextOffsetX = 200;//「Now Loading...」の位置(画面の右下から)
	constexpr int kLoadingTextOffsetY = 50;
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

	m_menu.Init();
	m_menu.SetTitle("ステージセレクト");
	m_menu.SetListHeader("ステージ一覧");
	std::vector<std::string> items;
	for (int i = 0; i < static_cast<int>(StageType::Num); ++i)
	{
		items.push_back(StageData::GetInfo(static_cast<StageType>(i)).name);
	}
	m_menu.SetItems(items);
	ApplyCursor();
}

void StageSelectScene::ApplyCursor()
{
	const StageInfo& info = StageData::GetInfo(static_cast<StageType>(s_cursor));
	m_menu.SetCursor(s_cursor);
	m_menu.SetDetail("詳細", info.name, info.description);
}

StageSelectScene::~StageSelectScene()
{
}

void StageSelectScene::Update()
{
	m_menu.Update();
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
	ApplyCursor();

	//Yでチュートリアルを上に積む
	if (input.IsTriggered("Y"))
	{
		m_controller.PushScene(std::make_shared<TutorialScene>(m_controller, "Move"));
		return;
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
		//真っ黒になったら「Now Loading」を1回出す(読み込みは次のフレームの SceneMain::Init で行う)
		m_updateFunc = static_cast<UpdateFunc_t>(&StageSelectScene::LoadingUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&StageSelectScene::LoadingDraw);
	}
}

void StageSelectScene::LoadingUpdate()
{
	//このシーンはここで破棄されるので、呼んだ後は何もしない
	m_controller.ResetScene<SceneMain>(static_cast<StageType>(s_cursor));
}

void StageSelectScene::LoadingDraw()
{
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	DrawFormatString(Game::GetScreenWidth() - kLoadingTextOffsetX, Game::GetScreenHeight() - kLoadingTextOffsetY, GetColor(255, 255, 255), "Now Loading...");
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
	m_menu.Draw();
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
