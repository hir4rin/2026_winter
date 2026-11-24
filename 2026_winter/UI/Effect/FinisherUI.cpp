#include "FinisherUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include <algorithm>
#include <cmath>

namespace
{
	//原作(NINJA GAIDEN 4 の軍将の血殺)に合わせて、止め絵の最初のフレームから全部出し、割れるまで動かさない
	//切れる線(MakeBossCutStreak.ps1)//画面の上から下まで突き抜ける
	const char* const kCutPath = "data/UI/bloodUI/boss_cut_streak.png";
	constexpr float kCutImageWidth = 2048.0f;
	constexpr float kCutImageCenterY = 48.0f;
	constexpr float kCutThick = 0.75f;//太さ(設計解像度での倍率)
	constexpr float kCutOverScreen = 60.0f;//画面の外へはみ出す長さ(設計解像度)

	//画面の右端の縦の線の束
	const char* const kEdgePath = "data/UI/bloodUI/boss_edge_streaks.png";
	constexpr float kEdgeWidth = 270.0f;//画面での幅(設計解像度)//画像(400×1024)を画面の高さいっぱいに伸ばす

	constexpr float kCutFadeOutFrame = 15.0f;//血殺の文字と同じ(実フレーム)
}

FinisherUI::~FinisherUI()
{
	DeleteGraph(m_cutHandle);
	DeleteGraph(m_edgeHandle);
}

void FinisherUI::Init()
{
	m_isVisible = true;
	m_cutHandle = LoadGraph(kCutPath);
	m_edgeHandle = LoadGraph(kEdgePath);
}

void FinisherUI::Update()
{
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle)return;
	const auto& screen = battle->GetFinisherScreen();
	m_whiteRate = screen.whiteRate;

	if (screen.isCutLine && !m_wasCutLine)m_isCutFlip = GetRand(1) == 0;
	m_wasCutLine = screen.isCutLine;

	//血殺の間は出し、終わったら消す
	m_cutAlphaRate = screen.isCutLine ? 1.0f : (std::max)(0.0f, m_cutAlphaRate - 1.0f / kCutFadeOutFrame);
}

void FinisherUI::Draw()
{
	DrawCut();

	if (m_whiteRate <= 0.0f)return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * m_whiteRate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), 0xffffff, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void FinisherUI::DrawCut()
{
	if (m_cutAlphaRate <= 0.0f)return;
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle)return;
	const auto& screen = battle->GetFinisherScreen();
	const float scale = Game::GetScale();
	const float screenW = static_cast<float>(Game::GetScreenWidth());
	const float screenH = static_cast<float>(Game::GetScreenHeight());

	SetDrawMode(DX_DRAWMODE_BILINEAR);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * m_cutAlphaRate));

	//画面の右端の縦の線の束
	const float edgeW = kEdgeWidth * scale;
	DrawExtendGraphF(screenW - edgeW, 0.0f, screenW, screenH, m_edgeHandle, TRUE);

	//切れる線: 線の両端を画面に写し、その向きのまま画面の上から下まで伸ばす//カメラの後ろなら出さない
	const VECTOR top = ConvWorldPosToScreenPos(screen.cutTop.ToDxLibVector());
	const VECTOR bottom = ConvWorldPosToScreenPos(screen.cutBottom.ToDxLibVector());
	const bool isInFront = top.z > 0.0f && top.z < 1.0f && bottom.z > 0.0f && bottom.z < 1.0f;
	const float dx = bottom.x - top.x;
	const float dy = bottom.y - top.y;
	if (isInFront && dy > 1.0f)
	{
		//画面の上の端(y=-はみ出し)と下の端(y=高さ+はみ出し)で線を切る
		const float over = kCutOverScreen * scale;
		const float slope = dx / dy;
		const float startX = top.x + slope * (-over - top.y);
		const float endX = top.x + slope * (screenH + over - top.y);
		const float lenX = endX - startX;
		const float lenY = screenH + over * 2.0f;
		const float length = sqrtf(lenX * lenX + lenY * lenY);
		const float angle = atan2f(lenY, lenX);
		DrawRotaGraph3F(startX, -over, 0.0f, kCutImageCenterY, length / kCutImageWidth, kCutThick * scale, angle, m_cutHandle, TRUE, FALSE, m_isCutFlip);
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}
