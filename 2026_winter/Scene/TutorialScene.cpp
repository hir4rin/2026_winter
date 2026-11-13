#include "TutorialScene.h"
#include "SceneController.h"
#include "DxLib.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "DataLoader/DataManager.h"
#include <cassert>


namespace
{
	constexpr int kFadeFrame = 15;
	constexpr int kDarkenAlpha = 200;//下のシーンを暗くする

	//画像の枠
	constexpr int kImageX = 320;
	constexpr int kImageY = 140;
	constexpr int kImageW = 640;
	constexpr int kImageH = 360;
	constexpr int kTitleY = 90;
	constexpr int kTextY = 530;
	constexpr int kPageNumY = 600;
	constexpr int kGuideOffsetX = 400;//画面の右下から
	constexpr int kGuideOffsetY = 50;

	const std::string kTutorialDir = "data/Tutorial/";

}

TutorialScene::TutorialScene(SceneController& controller, const std::string& id) :Scene(controller)
{

	for (const auto& data : DataManager::GetInstance().GetTutorialPages(id))
	{
		Page page;
		page.handle = LoadGraph((kTutorialDir + data.file).c_str());
		page.title = data.title;
		page.text = data.text;
		m_pages.push_back(page);
	}
	assert(!m_pages.empty() && "Tutorial.csvにそのidがありません");

	m_updateFunc = static_cast<UpdateFunc_t>(&TutorialScene::FadeInUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&TutorialScene::FadeInDraw);
}

TutorialScene::~TutorialScene()
{
	for (auto& page : m_pages)
	{
		DeleteGraph(page.handle);
	}
}

void TutorialScene::Update()
{
	//下のシーンはUpdateされないので、ここで入力を更新する
	Input::GetInstance().Update();
	(this->*m_updateFunc)();
}

void TutorialScene::FadeInUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&TutorialScene::NormalUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&TutorialScene::NormalDraw);
	}
}

void TutorialScene::NormalUpdate()
{
	auto& input = Input::GetInstance();

	if (input.IsTriggered("Left") && m_pageIndex > 0)
	{
		--m_pageIndex;
	}
	if (input.IsTriggered("A") || input.IsTriggered("Right"))
	{
		if (m_pageIndex + 1 < static_cast<int>(m_pages.size()))
		{
			++m_pageIndex;
		}
		else
		{
			StartFadeOut();//最後のページで閉じる
		}
	}
	if (input.IsTriggered("B"))
	{
		StartFadeOut();
	}
}

void TutorialScene::StartFadeOut()
{
	m_fadeFrame = 0;
	m_updateFunc = static_cast<UpdateFunc_t>(&TutorialScene::FadeOutUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&TutorialScene::FadeOutDraw);
}

void TutorialScene::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		//このシーンは破棄されるので、呼んだ後は何もしない
		m_controller.PopScene();
	}
}

void TutorialScene::Draw()
{
	(this->*m_drawFunc)();
}

void TutorialScene::FadeInDraw()
{
	DrawContent(static_cast<float>(m_fadeFrame) / kFadeFrame);
}

void TutorialScene::NormalDraw()
{
	DrawContent(1.0f);
}

void TutorialScene::FadeOutDraw()
{
	DrawContent(1.0f - static_cast<float>(m_fadeFrame) / kFadeFrame);
}

void TutorialScene::DrawContent(float alpha)
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kDarkenAlpha * alpha));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);

	const Page& page = m_pages[m_pageIndex];
	const unsigned int white = GetColor(255, 255, 255);
	const bool isLast = (m_pageIndex + 1 == static_cast<int>(m_pages.size()));

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	DrawString(kImageX, kTitleY, page.title.c_str(), white);
	DrawExtendGraph(kImageX, kImageY, kImageX + kImageW, kImageY + kImageH, page.handle, TRUE);
	DrawString(kImageX, kTextY, page.text.c_str(), white);
	DrawFormatString(kImageX, kPageNumY, white, "%d / %d", m_pageIndex + 1, static_cast<int>(m_pages.size()));
	DrawString(Game::GetScreenWidth() - kGuideOffsetX, Game::GetScreenHeight() - kGuideOffsetY,
		isLast ? "A/B:閉じる  ←:前へ" : "A/→:次へ  ←:前へ  B:閉じる", white);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
