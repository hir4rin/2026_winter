#include "TrainingMenuUI.h"
#include "DxLib.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
	//色(MenuUIと同じ)
	constexpr int kRedR = 230, kRedG = 45, kRedB = 70;
	constexpr int kShadeR = 10, kShadeG = 4, kShadeB = 8;
	//一覧の項目の位置(MenuUIと同じ)
	constexpr float kItemStartY = 172.0f;
	constexpr float kItemPitch = 44.0f;
	constexpr float kValueRightX = 408.0f;//>の右端
	constexpr float kValueOffsetY = 9.0f;//項目の文字(22)と小さい文字(17)の高さの差
	constexpr float kArrowGap = 2.0f;//値と＜＞の間
	//値が変わったとき
	constexpr int kChangeFrame = 8;
	constexpr float kChangeSlide = 14.0f;
	constexpr float kArrowPop = 6.0f;
	//選んでいる間の＜＞の揺れ
	constexpr float kArrowBob = 2.0f;
	constexpr float kArrowBobSpeed = 0.12f;
	constexpr int kArrowG = 170, kArrowB = 180;//＜＞の色(赤い帯の上で見えるように薄い赤)
	//選択肢の点
	constexpr float kPipY = 36.0f;//項目の上からの高さ
	constexpr float kPipPitch = 9.0f;
	constexpr float kPipSize = 2.5f;
	constexpr int kPipMaxNum = 5;//これより多いと点が枠からはみ出すのでバーにする
	constexpr float kBarHalfHeight = 1.5f;
	//まだ反映していない印
	constexpr float kChangedMarkX = 83.0f;//項目の左の括弧と文字の間
	constexpr float kChangedMarkRadius = 3.0f;
	constexpr float kMarkOffsetY = 9.0f;//小さい文字の真ん中
	//右上の説明
	constexpr float kHelpX1 = 860.0f, kHelpY1 = 40.0f, kHelpX2 = 1222.0f, kHelpY2 = 300.0f;
	constexpr float kHelpPad = 16.0f;
	constexpr float kHelpTextY = 52.0f;
	constexpr float kHelpLineHeight = 28.0f;
	constexpr float kDirtyY = 252.0f;
	constexpr float kDirtyPulseSpeed = 0.1f;
	constexpr float kListFillInset = 4.0f;
	constexpr int kPanelFillAlpha = 210;
	//下のボタン説明(PauseUIと同じ位置)
	constexpr float kGuideY = 676.0f;
	constexpr float kGuideX = 62.0f;
	constexpr float kGuideIconSize = 24.0f;
	constexpr float kGuideTextGap = 6.0f;
	constexpr float kGuideItemGap = 28.0f;
}

void TrainingMenuUI::Update()
{
	PauseUI::Update();
	if (m_changeRow >= 0 && m_changeFrame < kChangeFrame) m_changeFrame++;
}

void TrainingMenuUI::StartValueChange(int row, int dir)
{
	if (row < 0 || row >= static_cast<int>(m_values.size()))return;
	m_changeOldText = m_values[row].text;
	m_changeRow = row;
	m_changeDir = dir;
	m_changeFrame = 0;
}

void TrainingMenuUI::DrawDescription()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	for (int i = 0; i < static_cast<int>(m_values.size()) && i < static_cast<int>(m_items.size()); ++i)
	{
		if (m_values[i].text.empty())continue;
		DrawValue(i, kItemStartY + kItemPitch * i);
	}
}

void TrainingMenuUI::DrawValue(int row, float y)
{
	const ValueRow& value = m_values[row];
	const bool isSelect = (row == m_cursor);
	const int arrowW = GetDrawStringWidthToHandle(">", static_cast<int>(strlen(">")), m_smallFont);
	const int slotW = GetDrawStringWidthToHandle(m_slotWidest.c_str(), static_cast<int>(m_slotWidest.size()), m_smallFont);
	const int slotRight = ToScreenX(kValueRightX) - arrowW - static_cast<int>(kArrowGap);
	const int slotLeft = slotRight - slotW;
	const float slotCenter = (slotLeft + slotRight) * 0.5f;
	const int textY = ToScreenY(y + kValueOffsetY);
	const int leftArrowX = slotLeft - static_cast<int>(kArrowGap) - arrowW;

	//値の文字(枠の真ん中。変わった直後は、新しい値が押した向きから入り、前の値が反対へ抜ける)
	const bool isChanging = (row == m_changeRow && m_changeFrame < kChangeFrame);
	const float t = isChanging ? static_cast<float>(m_changeFrame) / kChangeFrame : 1.0f;
	const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	const unsigned int textColor = isSelect ? GetColor(255, 236, 238) : GetColor(220, 150, 160);
	const int textW = GetDrawStringWidthToHandle(value.text.c_str(), static_cast<int>(value.text.size()), m_smallFont);
	const float inX = m_changeDir * kChangeSlide * (1.0f - ease);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(static_cast<int>(255 * ease)));
	DrawStringToHandle(static_cast<int>(slotCenter - textW * 0.5f + inX), textY, value.text.c_str(), textColor, m_smallFont);
	if (isChanging)
	{
		const int oldW = GetDrawStringWidthToHandle(m_changeOldText.c_str(), static_cast<int>(m_changeOldText.size()), m_smallFont);
		const float outX = -m_changeDir * kChangeSlide * ease;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(static_cast<int>(255 * (1.0f - ease))));
		DrawStringToHandle(static_cast<int>(slotCenter - oldW * 0.5f + outX), textY, m_changeOldText.c_str(), textColor, m_smallFont);
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));

	//まだ反映していない項目は、項目名の左に点
	if (value.isChanged)
	{
		const float markX = static_cast<float>(ToScreenX(kChangedMarkX));
		const float markY = static_cast<float>(ToScreenY(y + kValueOffsetY + kMarkOffsetY));
		const unsigned int markColor = isSelect ? GetColor(255, 236, 238) : GetColor(kRedR, kRedG, kRedB);
		DrawCircleAA(markX, markY, kChangedMarkRadius, 16, markColor, TRUE);
	}
	if (!isSelect)return;

	//＜＞(ゆっくり外へ揺れる。押した側は外へ跳ねて白く光る)
	const float bob = kArrowBob * (0.5f + 0.5f * std::sin(m_frame * kArrowBobSpeed));
	const float pop = isChanging ? kArrowPop * (1.0f - ease) : 0.0f;
	const auto arrowColor = [&](int side)
		{
			const float w = (isChanging && side == m_changeDir) ? 1.0f - ease : 0.0f;
			return GetColor(255, kArrowG + static_cast<int>((255 - kArrowG) * w), kArrowB + static_cast<int>((255 - kArrowB) * w));
		};
	const float leftOut = bob + (m_changeDir < 0 ? pop : 0.0f);
	const float rightOut = bob + (m_changeDir > 0 ? pop : 0.0f);
	DrawStringToHandle(leftArrowX - static_cast<int>(leftOut), textY, "<", arrowColor(-1), m_smallFont);
	DrawStringToHandle(ToScreenX(kValueRightX) - arrowW + static_cast<int>(rightOut), textY, ">", arrowColor(1), m_smallFont);

	//選択肢の数と今の位置(枠の真ん中に固定。値を変えても動かない)
	if (value.num <= 1)return;
	const float pipCenterY = static_cast<float>(ToScreenY(y + kPipY));
	if (value.num > kPipMaxNum)
	{
		//枠の幅のバーを今の位置まで塗る
		const float barLeft = static_cast<float>(slotLeft);
		const float barRight = static_cast<float>(slotRight);
		const float rate = static_cast<float>(value.index) / static_cast<float>(value.num - 1);
		const float fillRight = barLeft + (barRight - barLeft) * rate;
		DrawBoxAA(barLeft, pipCenterY - kBarHalfHeight, barRight, pipCenterY + kBarHalfHeight, GetColor(120, 60, 70), TRUE);
		DrawBoxAA(barLeft, pipCenterY - kBarHalfHeight, fillRight, pipCenterY + kBarHalfHeight, GetColor(255, 236, 238), TRUE);
		const float knob = kPipSize * 1.3f;
		DrawBoxAA(fillRight - knob, pipCenterY - knob, fillRight + knob, pipCenterY + knob, GetColor(255, 236, 238), TRUE);
		return;
	}
	const float pipLeft = slotCenter - kPipPitch * (value.num - 1) * 0.5f;
	for (int i = 0; i < value.num; ++i)
	{
		const float x = pipLeft + kPipPitch * i;
		const bool isNow = (i == value.index);
		const float size = isNow ? kPipSize * 1.3f : kPipSize * 0.8f;
		const unsigned int color = isNow ? GetColor(255, 236, 238) : GetColor(120, 60, 70);
		DrawBoxAA(x - size, pipCenterY - size, x + size, pipCenterY + size, color, TRUE);
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
		const float pulse = 0.65f + 0.35f * std::sin(m_frame * kDirtyPulseSpeed);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(static_cast<int>(255 * pulse)));
		DrawStringToHandle(ToScreenX(kHelpX1 + kHelpPad + 8.0f), ToScreenY(kDirtyY), "● 閉じると反映する",
			GetColor(kRedR, kRedG, kRedB), m_smallFont);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	}
}

void TrainingMenuUI::DrawPauseGuide()
{
	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	const float iconY = kGuideY - kGuideIconSize * 0.5f;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, Alpha(255));
	//左から順に並べる(戻るの文字の長さが変わるので位置は文字の幅で決める)
	float x = kGuideX;
	const auto drawItem = [&](int icon, const char* text)
		{
			if (icon >= 0)
			{
				DrawExtendGraph(ToScreenX(x), ToScreenY(iconY), ToScreenX(x + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), icon, TRUE);
				x += kGuideIconSize + kGuideTextGap;
			}
			DrawStringToHandle(ToScreenX(x), ToScreenY(iconY + 2.0f), text, red, m_smallFont);
			x += static_cast<float>(GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), m_smallFont)) + kGuideItemGap;
		};
	drawItem(m_buttonAHandle, "決定");
	drawItem(m_buttonBHandle, m_isDirty ? "反映して戻る" : "戻る");
	drawItem(-1, "←→ 変更");
	drawItem(-1, "↑↓ 選択");
}
