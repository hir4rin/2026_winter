#include "ResultScene.h"
#include <fstream>
#include <map>
#include <filesystem>
#include "SceneController.h"
#include "DxLib.h"
#include "StageSelectScene.h"
#include "SceneMain.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"

namespace
{
	constexpr int kFadeFrame = 30;//フェードにかけるフレーム数
	const char* const kSaveDir = "Save";
	const char* const kBestScorePath = "Save/BestScore.txt";
	constexpr int kLoadingTextOffsetX = 200;//「Now Loading...」の位置(画面の右下から)
	constexpr int kLoadingTextOffsetY = 50;
}

ResultScene::ResultScene(SceneController& controller, StageType stageType, const ResultData& data, bool isSaveRecord) :
	Scene(controller),
	m_stageType(stageType)
{
	//スローのまま来ても戻す
	System::GetInstance().SetTimeScale(1.0f);
	//-result で直接開いたときのため
	Input::GetInstance().Init();
	Input::GetInstance().SetInputBlocked(false);

	const int total = data.CalcTotalScore();
	const int best = LoadBestScore(data.stageType);
	const bool isNewRecord = total > best;
	if (isNewRecord && isSaveRecord) SaveBestScore(data.stageType, total);

	m_ui.Init(data, StageData::GetInfo(stageType).name, isNewRecord);

	m_updateFunc = static_cast<UpdateFunc_t>(&ResultScene::FadeInUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&ResultScene::FadeInDraw);
}

ResultScene::~ResultScene()
{
}

ResultData ResultScene::MakeDebugData()
{
	using T = ResultData::Technique;
	ResultData data;
	data.stageType = static_cast<int>(StageType::Cyber);
	data.clearFrame = 252 * 60 + 21;
	data.killCount = 24;
	data.enemyNum = 30;
	data.maxCombo = 42;
	data.techniqueCounts[static_cast<int>(T::PartBreak)] = 3;
	data.techniqueCounts[static_cast<int>(T::PartBrokenKill)] = 2;
	data.techniqueCounts[static_cast<int>(T::Assasin)] = 2;
	data.techniqueCounts[static_cast<int>(T::Skill)] = 5;
	data.techniqueCounts[static_cast<int>(T::SkillKessatsu)] = 1;
	return data;
}

void ResultScene::Update()
{
	(this->*m_updateFunc)();
}

void ResultScene::FadeInUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame / 2)
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
	m_ui.Update();

	//演出中はAで飛ばす
	if (!m_ui.IsFinished())
	{
		if (input.IsTriggered("A")) m_ui.Skip();
		return;
	}

	const bool isNext = input.IsTriggered("A");
	m_isRetry = input.IsTriggered("Y");
	if (isNext || m_isRetry)
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&ResultScene::FadeOutUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&ResultScene::FadeOutDraw);
	}
}

void ResultScene::FadeOutUpdate()
{
	m_ui.Update();
	m_fadeFrame++;
	if (m_fadeFrame < kFadeFrame) return;
	if (m_isRetry)
	{
		//真っ黒の「Now Loading」を1回出してから読む
		m_updateFunc = static_cast<UpdateFunc_t>(&ResultScene::LoadingUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&ResultScene::LoadingDraw);
		return;
	}
	//このシーンはここで破棄されるので、呼んだ後は何もしない
	m_controller.ResetScene<StageSelectScene>();
}

void ResultScene::LoadingUpdate()
{
	m_controller.ResetScene<SceneMain>(m_stageType);
}

void ResultScene::LoadingDraw()
{
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	DrawFormatString(Game::GetScreenWidth() - kLoadingTextOffsetX, Game::GetScreenHeight() - kLoadingTextOffsetY, GetColor(255, 255, 255), "Now Loading...");
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
	m_ui.Draw();
}

void ResultScene::FadeOutDraw()
{
	NormalDraw();
	DrawFade();
}

void ResultScene::DrawFade()
{
	//フェードイン中は黒→透明、フェードアウト中は透明→黒
	const bool isFadeIn = m_updateFunc == static_cast<UpdateFunc_t>(&ResultScene::FadeInUpdate);
	const int frame = isFadeIn ? kFadeFrame / 2 : kFadeFrame;
	float rate = static_cast<float>(m_fadeFrame) / static_cast<float>(frame);
	if (isFadeIn) rate = 1.0f - rate;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

int ResultScene::LoadBestScore(int stage)
{
	std::ifstream file(kBestScorePath);
	int s = 0;
	int score = 0;
	while (file >> s >> score)
	{
		if (s == stage) return score;
	}
	return 0;
}

void ResultScene::SaveBestScore(int stage, int score)
{
	std::map<int, int> scores;
	{
		std::ifstream file(kBestScorePath);
		int s = 0;
		int value = 0;
		while (file >> s >> value) scores[s] = value;
	}
	scores[stage] = score;
	std::filesystem::create_directories(kSaveDir);
	std::ofstream file(kBestScorePath);
	for (const auto& [s, value] : scores) file << s << " " << value << "\n";
}
