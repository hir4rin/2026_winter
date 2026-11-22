#include "TitleFlashUI.h"
#include "DxLib.h"
#include "Main/Game.h"

namespace
{
	constexpr int kFlashTime = 10;
	constexpr int kFlashAlpha = 170;
}

void TitleFlashUI::Init()
{
	m_isVisible = false;
}

void TitleFlashUI::Update()
{
	if (!m_isVisible)return;
	m_frame++;
	if (m_frame >= kFlashTime)m_isVisible = false;
}

void TitleFlashUI::Draw()
{
	if (!m_isVisible)return;
	const float rate = 1.0f - static_cast<float>(m_frame) / kFlashTime;
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kFlashAlpha * rate * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(255, 235, 230), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TitleFlashUI::Start()
{
	m_frame = 0;
	m_isVisible = true;
}
