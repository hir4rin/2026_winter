#include "TitlePressUI.h"
#include "DxLib.h"
#include "UI/UIFont.h"
#include "Main/Game.h"
#include <cmath>
#include <cstring>

namespace
{
	const char* const kText = "PRESS  A  BUTTON";
	constexpr float kX = 530.0f;//ロゴの下
	constexpr float kY = 660.0f;
	constexpr int kFontSize = 22;
	constexpr int kSpace = 8;
	constexpr float kBlinkSpeed = 0.06f;
}

TitlePressUI::~TitlePressUI()
{
	DeleteFontToHandle(m_font);
}

void TitlePressUI::Init()
{
	const float scale = Game::GetScale();
	m_font = UIFont::Create(static_cast<int>(kFontSize * scale), 3, DX_FONTTYPE_ANTIALIASING_4X4);
	SetFontSpaceToHandle(static_cast<int>(kSpace * scale), m_font);
	m_isVisible = false;
}

void TitlePressUI::Update()
{
	if (m_isVisible)m_frame++;
}

void TitlePressUI::Draw()
{
	if (!m_isVisible)return;
	//ゆっくり明滅
	const float wave = 0.5f - 0.5f * std::cos(m_frame * kBlinkSpeed);
	const int alpha = static_cast<int>(40 + 215 * wave);

	const int width = GetDrawStringWidthToHandle(kText, static_cast<int>(strlen(kText)), m_font);
	const int size = GetFontSizeToHandle(m_font);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringToHandle(static_cast<int>(Game::ScaleX(kX)) - width / 2, static_cast<int>(Game::ScaleY(kY)) - size / 2,
		kText, GetColor(225, 222, 218), m_font);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TitlePressUI::Show()
{
	if (m_isVisible)return;
	m_frame = 0;
	m_isVisible = true;
}
