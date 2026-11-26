#include "TrainingMenuUI.h"
#include "DxLib.h"
#include <cstring>

namespace
{
	//色(MenuUIと同じ)
	constexpr int kRedR = 230, kRedG = 45, kRedB = 70;
	constexpr int kShadeR = 10, kShadeG = 4, kShadeB = 8;
	//一覧の項目の位置(MenuUIと同じ)
	constexpr float kItemStartY = 172.0f;
	constexpr float kItemPitch = 44.0f;
	constexpr float kValueRightX = 402.0f;//値の右端
	constexpr float kValueOffsetY = 9.0f;//項目の文字(22)と小さい文字(17)の高さの差
	constexpr float kArrowGap = 10.0f;//値と＜＞の間
	//右上の説明
	constexpr float kHelpX1 = 860.0f, kHelpY1 = 40.0f, kHelpX2 = 1222.0f, kHelpY2 = 300.0f;
	constexpr float kHelpPad = 16.0f;
	constexpr float kHelpTextY = 52.0f;
	constexpr float kHelpLineHeight = 28.0f;
	constexpr float kDirtyY = 252.0f;
	constexpr float kListFillInset = 4.0f;
	constexpr int kPanelFillAlpha = 210;
	//下のボタン説明(PauseUIと同じ位置)
	constexpr float kGuideY = 676.0f;
	constexpr float kGuideX = 62.0f;
	constexpr float kGuideIconSize = 24.0f;
	constexpr float kGuideBX = 157.0f;
	constexpr float kGuideChangeX = 257.0f;
	constexpr float kGuideSelectX = 380.0f;
}

void TrainingMenuUI::DrawDescription()
{
	const unsigned int valueColor = GetColor(220, 150, 160);
	const unsigned int selectColor = GetColor(255, 236, 238);
	const unsigned int arrowColor = GetColor(kRedR, kRedG, kRedB);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	for (int i = 0; i < static_cast<int>(m_values.size()) && i < static_cast<int>(m_items.size()); ++i)
	{
		const std::string& value = m_values[i];
		if (value.empty())continue;
		const float y = kItemStartY + kItemPitch * i + kValueOffsetY;
		const int w = GetDrawStringWidthToHandle(value.c_str(), static_cast<int>(value.size()), m_smallFont);
		if (i != m_cursor)
		{
			DrawStringToHandle(ToScreenX(kValueRightX) - w, ToScreenY(y), value.c_str(), valueColor, m_smallFont);
			continue;
		}
		//選んでいる項目は ＜ 値 ＞
		const int arrowW = GetDrawStringWidthToHandle("＞", static_cast<int>(strlen("＞")), m_smallFont);
		const int right = ToScreenX(kValueRightX) - arrowW - static_cast<int>(kArrowGap);
		DrawStringToHandle(ToScreenX(kValueRightX) - arrowW, ToScreenY(y), "＞", arrowColor, m_smallFont);
		DrawStringToHandle(right - w, ToScreenY(y), value.c_str(), selectColor, m_smallFont);
		DrawStringToHandle(right - w - static_cast<int>(kArrowGap) - arrowW, ToScreenY(y), "＜", arrowColor, m_smallFont);
	}
}

void TrainingMenuUI::DrawRecord()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(kPanelFillAlpha));
	DrawBox(ToScreenX(kHelpX1 + kListFillInset), ToScreenY(kHelpY1 + kListFillInset),
		ToScreenX(kHelpX2 - kListFillInset), ToScreenY(kHelpY2 - kListFillInset), GetColor(kShadeR, kShadeG, kShadeB), TRUE);
	DrawFrame(kHelpX1, kHelpY1, kHelpX2, kHelpY2, kRedR, kRedG, kRedB);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	DrawHeaderLine(kHelpX1 + kHelpPad, kHelpY1 + 10.0f, kHelpX2 - kHelpPad, "説明");
	DrawWrapText(kHelpX1 + kHelpPad + 8.0f, kHelpY1 + kHelpTextY, kHelpX2 - kHelpX1 - kHelpPad * 2.0f - 16.0f, kHelpLineHeight,
		m_detailText, GetColor(255, 236, 238), m_smallFont);
	if (m_isDirty)
	{
		DrawStringToHandle(ToScreenX(kHelpX1 + kHelpPad + 8.0f), ToScreenY(kDirtyY), "● 閉じたときに反映して敵を出し直す",
			GetColor(kRedR, kRedG, kRedB), m_smallFont);
	}
}

void TrainingMenuUI::DrawPauseGuide()
{
	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	const float iconY = kGuideY - kGuideIconSize * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	DrawExtendGraph(ToScreenX(kGuideX), ToScreenY(iconY), ToScreenX(kGuideX + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), m_buttonAHandle, TRUE);
	DrawStringToHandle(ToScreenX(kGuideX + kGuideIconSize + 6.0f), ToScreenY(iconY + 2.0f), "決定", red, m_smallFont);
	DrawExtendGraph(ToScreenX(kGuideBX), ToScreenY(iconY), ToScreenX(kGuideBX + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), m_buttonBHandle, TRUE);
	DrawStringToHandle(ToScreenX(kGuideBX + kGuideIconSize + 6.0f), ToScreenY(iconY + 2.0f), "戻る", red, m_smallFont);
	DrawStringToHandle(ToScreenX(kGuideChangeX), ToScreenY(iconY + 2.0f), "←→ 変更", red, m_smallFont);
	DrawStringToHandle(ToScreenX(kGuideSelectX), ToScreenY(iconY + 2.0f), "↑↓ 選択", red, m_smallFont);
}
