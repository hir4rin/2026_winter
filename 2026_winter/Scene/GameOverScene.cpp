#include "GameOverScene.h"
#include "SceneController.h"
#include "SceneMain.h"
#include "DxLib.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include <algorithm>
#include <cmath>

namespace
{
	const char* const kShiPath = "data/UI/Death/shi.png";
	const char* const kShiDripPath = "data/UI/Death/shi_drip.png";
	const char* const kVignettePath = "data/UI/bloodUI/ink_vignette.png";
	const char* const kButtonAPath = "data/UI/button/A.png";
	const char* const kFontName = "游明朝";

	//暗くする(実フレーム)
	constexpr int kDarkenFrame = 40;
	constexpr int kDarkenAlpha = 150;

	//「死」(座標は設計解像度)
	constexpr float kShiCenterX = 640.0f;
	constexpr float kShiCenterY = 330.0f;
	constexpr float kShiScale = 0.9f;
	constexpr int kShiSize = 512;//画像の大きさ
	constexpr int kShiStart = 16;
	constexpr int kShiInFrame = 14;
	constexpr float kShiStartScale = 1.3f;

	//垂れ(画像の322pxから下)
	constexpr int kDripTop = 322;
	constexpr int kDripStart = 40;
	constexpr int kDripFrame = 28;

	//ボタン説明
	constexpr int kGuideStart = 84;
	constexpr int kGuideInFrame = 20;
	constexpr float kGuideY = 640.0f;
	constexpr float kButtonSize = 40.0f;
	constexpr float kGuideGap = 12.0f;
	constexpr int kFontSize = 28;
	constexpr float kGuideBlinkSpeed = 0.06f;
	constexpr float kGuideBlinkMin = 0.65f;

	constexpr int kFadeOutFrame = 30;
}

GameOverScene::GameOverScene(SceneController& controller, StageType stageType, const RespawnInfo& respawn) :
	Scene(controller),
	m_stageType(stageType),
	m_respawn(respawn)
{
	m_shiHandle = LoadGraph(kShiPath);
	m_shiDripHandle = LoadGraph(kShiDripPath);
	m_vignetteHandle = LoadGraph(kVignettePath);
	m_buttonAHandle = LoadGraph(kButtonAPath);
	m_fontHandle = CreateFontToHandle(kFontName, static_cast<int>(kFontSize * Game::GetScale()), 3, DX_FONTTYPE_ANTIALIASING_8X8);

	m_updateFunc = static_cast<UpdateFunc_t>(&GameOverScene::FadeInUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&GameOverScene::FadeInDraw);
}

GameOverScene::~GameOverScene()
{
	DeleteGraph(m_shiHandle);
	DeleteGraph(m_shiDripHandle);
	DeleteGraph(m_vignetteHandle);
	DeleteGraph(m_buttonAHandle);
	DeleteFontToHandle(m_fontHandle);
}

void GameOverScene::Update()
{
	//下のSceneMainはUpdateされないので、ここで入力を更新する(イベント中の入力止めも外す)
	auto& input = Input::GetInstance();
	input.SetInputBlocked(false);
	input.Update();

	m_frame++;
	(this->*m_updateFunc)();
}

void GameOverScene::FadeInUpdate()
{
	if (m_frame >= kGuideStart)
	{
		m_updateFunc = static_cast<UpdateFunc_t>(&GameOverScene::NormalUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&GameOverScene::NormalDraw);
	}
}

void GameOverScene::NormalUpdate()
{
	if (Input::GetInstance().IsTriggered("A"))
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&GameOverScene::FadeOutUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&GameOverScene::FadeOutDraw);
	}
}

void GameOverScene::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeOutFrame)
	{
		//下のSceneMainごと破棄して、続きから作り直す(このシーンも破棄されるので、呼んだ後は何もしない)
		m_controller.ResetScene<SceneMain>(m_stageType, std::optional<RespawnInfo>(m_respawn));
	}
}

void GameOverScene::Draw()
{
	(this->*m_drawFunc)();
}

void GameOverScene::FadeInDraw()
{
	DrawContent();
}

void GameOverScene::NormalDraw()
{
	DrawContent();
}

void GameOverScene::FadeOutDraw()
{
	DrawContent();

	const float rate = std::clamp(static_cast<float>(m_fadeFrame) / kFadeOutFrame, 0.0f, 1.0f);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void GameOverScene::DrawContent()
{
	const float darkRate = std::clamp(static_cast<float>(m_frame) / kDarkenFrame, 0.0f, 1.0f);

	SetDrawMode(DX_DRAWMODE_BILINEAR);

	//下のゲームを暗くして、ふちを墨で沈める
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kDarkenAlpha * darkRate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * darkRate));
	DrawExtendGraph(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), m_vignetteHandle, TRUE);

	DrawShi();
	DrawGuide();

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

void GameOverScene::DrawShi()
{
	const int frame = m_frame - kShiStart;
	if (frame < 0) return;

	//大きい所から叩きつけるように縮む
	const float t = std::clamp(static_cast<float>(frame) / kShiInFrame, 0.0f, 1.0f);
	const float ease = 1.0f - std::pow(1.0f - t, 3.0f);
	const float scale = kShiScale * Game::GetScale() * (kShiStartScale + (1.0f - kShiStartScale) * ease);
	const float cx = Game::ScaleX(kShiCenterX);
	const float cy = Game::ScaleY(kShiCenterY);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * t));
	DrawRotaGraphF(cx, cy, scale, 0.0, m_shiHandle, TRUE);

	//垂れを上から下へ伸ばす
	const float dripRate = std::clamp(static_cast<float>(m_frame - kDripStart) / kDripFrame, 0.0f, 1.0f);
	const int srcH = kDripTop + static_cast<int>((kShiSize - kDripTop) * dripRate);
	const float half = kShiSize * scale * 0.5f;
	DrawRectExtendGraphF(cx - half, cy - half, cx + half, cy - half + srcH * scale,
		0, 0, kShiSize, srcH, m_shiDripHandle, TRUE);
}

void GameOverScene::DrawGuide()
{
	const int frame = m_frame - kGuideStart;
	if (frame < 0) return;

	const float inRate = std::clamp(static_cast<float>(frame) / kGuideInFrame, 0.0f, 1.0f);
	const float blink = kGuideBlinkMin + (1.0f - kGuideBlinkMin) * (0.5f + 0.5f * std::cos(frame * kGuideBlinkSpeed));
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * inRate * blink));

	const char* const text = "再開";
	const float scale = Game::GetScale();
	const float buttonSize = kButtonSize * scale;
	const float gap = kGuideGap * scale;
	const int textW = GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), m_fontHandle);
	const float left = Game::ScaleX(kShiCenterX) - (buttonSize + gap + textW) * 0.5f;
	const float cy = Game::ScaleY(kGuideY);

	DrawExtendGraphF(left, cy - buttonSize * 0.5f, left + buttonSize, cy + buttonSize * 0.5f, m_buttonAHandle, TRUE);
	DrawStringFToHandle(left + buttonSize + gap, cy - GetFontSizeToHandle(m_fontHandle) * 0.5f,
		text, GetColor(232, 224, 214), m_fontHandle);
}
