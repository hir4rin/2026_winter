#include "PauseUI.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include "DxLib.h"
#include "Main/Game.h"

namespace
{
	const char* const kButtonBPath = "data/UI/button/B.png";

	//色(MenuUIと同じ)
	constexpr int kRedR = 230, kRedG = 45, kRedB = 70;
	constexpr int kShadeR = 10, kShadeG = 4, kShadeB = 8;

	//後ろを暗くする(左ほど濃い)
	constexpr int kShadeStopNum = 4;
	constexpr float kShadeStopX[kShadeStopNum] = { 0.0f, 0.38f, 0.62f, 1.0f };
	constexpr float kShadeStopAlpha[kShadeStopNum] = { 0.92f, 0.80f, 0.35f, 0.45f };
	constexpr int kShadeInFrame = 6;

	//左だけに流すドットの雨と格子
	constexpr float kEffectRight = 520.0f;
	constexpr float kRainTile = 256.0f;
	constexpr float kRainSpeed = 0.004f;
	constexpr int kRainAlpha = 110;
	constexpr float kGridTile = 64.0f;
	constexpr int kGridAlpha = 10;

	//タイトル(一覧の上)
	constexpr float kTitleX1 = 57.0f, kTitleX2 = 417.0f;

	//一覧(MenuUIの左のパネルと同じ位置)
	constexpr float kListX1 = 57.0f, kListY1 = 117.0f, kListX2 = 417.0f, kListY2 = 623.0f;
	constexpr float kListFillInset = 4.0f;
	constexpr int kListFillAlpha = 210;
	constexpr float kItemStartY = 172.0f;
	constexpr float kItemPitch = 44.0f;

	//一覧の下の説明
	constexpr float kDescLineX1 = 75.0f, kDescLineX2 = 399.0f;
	constexpr float kDescGap = 18.0f;
	constexpr float kDescX = 83.0f;
	constexpr float kDescWidth = 310.0f;
	constexpr float kDescLineHeight = 28.0f;

	//右上の記録
	constexpr float kRecordX1 = 900.0f, kRecordY1 = 40.0f, kRecordX2 = 1222.0f;
	constexpr float kRecordPad = 16.0f;
	constexpr float kRecordRowY = 46.0f;
	constexpr float kRecordRowPitch = 34.0f;
	constexpr int kRecordRowNum = 3;
	constexpr float kRecordBottomPad = 12.0f;

	//下のボタン説明
	constexpr float kGuideY = 676.0f;
	constexpr float kGuideX = 62.0f;
	constexpr float kGuideIconSize = 24.0f;
	constexpr float kGuideBX = 157.0f;
	constexpr float kGuideSelectX = 257.0f;

	//開く演出
	constexpr int kOpenFrame = 10;
	constexpr int kOpenFlickerFrame = 5;
	constexpr float kOpenSlide = 24.0f;
	constexpr int kListDelay = 2;
	constexpr int kRecordDelay = 5;
	constexpr int kGuideDelay = 6;

	constexpr int kFps = 60;
}

PauseUI::~PauseUI()
{
	DeleteGraph(m_buttonBHandle);
}

void PauseUI::Init()
{
	MenuUI::Init();
	m_buttonBHandle = LoadGraph(kButtonBPath);
	m_openFrame = 0;
}

void PauseUI::Update()
{
	MenuUI::Update();
	m_openFrame++;
}

void PauseUI::ApplyOpen(int delay, float dir)
{
	const int f = m_openFrame - delay;
	const float t = std::clamp(static_cast<float>(f) / kOpenFrame, 0.0f, 1.0f);
	const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	const float flicker = (f >= 0 && f < kOpenFlickerFrame && (f % 2) == 1) ? 0.35f : 1.0f;
	m_alpha = ease * flicker;
	m_offsetX = (1.0f - ease) * -kOpenSlide * dir;
}

void PauseUI::Draw()
{
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawShade();

	ApplyOpen(0, 0.0f);
	DrawTitle(kTitleX1, kTitleX2);

	ApplyOpen(kListDelay, 1.0f);
	if (m_alpha > 0.0f)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(kListFillAlpha));
		DrawBox(ToScreenX(kListX1 + kListFillInset), ToScreenY(kListY1 + kListFillInset),
			ToScreenX(kListX2 - kListFillInset), ToScreenY(kListY2 - kListFillInset), GetColor(kShadeR, kShadeG, kShadeB), TRUE);
		DrawListPanel();
		DrawDescription();
	}

	ApplyOpen(kRecordDelay, -1.0f);
	if (m_alpha > 0.0f) DrawRecord();

	ApplyOpen(kGuideDelay, 0.0f);
	if (m_alpha > 0.0f) DrawPauseGuide();

	m_alpha = 1.0f;
	m_offsetX = 0.0f;
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
}

void PauseUI::DrawShade()
{
	const float rate = (std::min)(1.0f, static_cast<float>(m_openFrame) / kShadeInFrame);
	const float sw = static_cast<float>(Game::GetScreenWidth());
	const float sh = static_cast<float>(Game::GetScreenHeight());

	//左から右へ薄くなる黒(区間ごとに頂点の色で補間)
	VERTEX2D vtx[(kShadeStopNum - 1) * 6];
	int n = 0;
	for (int i = 0; i < kShadeStopNum - 1; ++i)
	{
		const float x1 = sw * kShadeStopX[i];
		const float x2 = sw * kShadeStopX[i + 1];
		const COLOR_U8 c1 = GetColorU8(kShadeR, kShadeG, kShadeB, static_cast<int>(255 * kShadeStopAlpha[i] * rate));
		const COLOR_U8 c2 = GetColorU8(kShadeR, kShadeG, kShadeB, static_cast<int>(255 * kShadeStopAlpha[i + 1] * rate));
		const float px[6] = { x1, x2, x1, x2, x2, x1 };
		const float py[6] = { 0, 0, sh, 0, sh, sh };
		const COLOR_U8 pc[6] = { c1, c2, c1, c2, c2, c1 };
		for (int j = 0; j < 6; ++j)
		{
			vtx[n].pos = VGet(px[j], py[j], 0.0f);
			vtx[n].rhw = 1.0f;
			vtx[n].dif = pc[j];
			vtx[n].u = 0.0f;
			vtx[n].v = 0.0f;
			n++;
		}
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawPolygon2D(vtx, n / 3, DX_NONE_GRAPH, TRUE);

	//左の一覧の後ろだけに格子とドットの雨
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
	const float h = static_cast<float>(Game::kScreenHeight);
	DrawTiled(m_gridHandle, 0, 0, kEffectRight, h, kGridTile, 0.0f, 0.0f, kRedR, kRedG, kRedB, static_cast<int>(kGridAlpha * rate));
	DrawTiled(m_rainHandle, 0, 0, kEffectRight, h, kRainTile, 0.0f, -m_frame * kRainSpeed, kRedR, kRedG, kRedB, static_cast<int>(kRainAlpha * rate));
}

void PauseUI::DrawDescription()
{
	const float lineY = kItemStartY + kItemPitch * static_cast<float>(m_items.size()) + kDescGap;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	DrawFadeLine(kDescLineX1, lineY, kDescLineX2, lineY + 2.0f);
	DrawWrapText(kDescX, lineY + kDescGap, kDescWidth, kDescLineHeight, m_detailText, GetColor(220, 150, 160), m_smallFont);
}

void PauseUI::DrawRecord()
{
	const float y2 = kRecordY1 + kRecordRowY + kRecordRowPitch * kRecordRowNum + kRecordBottomPad;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(kListFillAlpha));
	DrawBox(ToScreenX(kRecordX1 + kListFillInset), ToScreenY(kRecordY1 + kListFillInset),
		ToScreenX(kRecordX2 - kListFillInset), ToScreenY(y2 - kListFillInset), GetColor(kShadeR, kShadeG, kShadeB), TRUE);
	DrawFrame(kRecordX1, kRecordY1, kRecordX2, y2, kRedR, kRedG, kRedB);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	DrawHeaderLine(kRecordX1 + kRecordPad, kRecordY1 + 10.0f, kRecordX2 - kRecordPad, "任務の記録");

	const int sec = m_record.elapsedFrame / kFps;
	char timeText[16];
	snprintf(timeText, sizeof(timeText), "%d:%02d", sec / 60, sec % 60);
	const std::string values[kRecordRowNum] = { m_record.stageName, timeText, std::to_string(m_record.killCount) };
	const char* const labels[kRecordRowNum] = { "任務", "経過", "撃破" };

	const unsigned int labelColor = GetColor(220, 150, 160);
	const unsigned int valueColor = GetColor(255, 236, 238);
	const int rightX = ToScreenX(kRecordX2 - kRecordPad - 8.0f);
	for (int i = 0; i < kRecordRowNum; ++i)
	{
		const float y = kRecordY1 + kRecordRowY + kRecordRowPitch * i;
		DrawStringToHandle(ToScreenX(kRecordX1 + kRecordPad + 8.0f), ToScreenY(y), labels[i], labelColor, m_smallFont);
		const int w = GetDrawStringWidthToHandle(values[i].c_str(), static_cast<int>(values[i].size()), m_smallFont);
		DrawStringToHandle(rightX - w, ToScreenY(y), values[i].c_str(), valueColor, m_smallFont);
	}
}

void PauseUI::DrawPauseGuide()
{
	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	const float iconY = kGuideY - kGuideIconSize * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	DrawExtendGraph(ToScreenX(kGuideX), ToScreenY(iconY), ToScreenX(kGuideX + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), m_buttonAHandle, TRUE);
	DrawStringToHandle(ToScreenX(kGuideX + kGuideIconSize + 6.0f), ToScreenY(iconY + 2.0f), "決定", red, m_smallFont);
	DrawExtendGraph(ToScreenX(kGuideBX), ToScreenY(iconY), ToScreenX(kGuideBX + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), m_buttonBHandle, TRUE);
	DrawStringToHandle(ToScreenX(kGuideBX + kGuideIconSize + 6.0f), ToScreenY(iconY + 2.0f), "戻る", red, m_smallFont);
	DrawStringToHandle(ToScreenX(kGuideSelectX), ToScreenY(iconY + 2.0f), "↑↓ 選択", red, m_smallFont);
}
