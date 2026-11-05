#include "FinisherUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include <cmath>

namespace
{
	constexpr float kBarHeightRate = 0.1f;//黒帯の高さ(画面の高さに対して)
	constexpr float kBarSpeed = 0.06f;
	constexpr float kSlashAngle = -30.0f * DX_PI_F / 180.0f;//斬撃の線の傾き
	constexpr float kSlashWidthRate = 0.05f;//線の太さ(画面の高さに対して)
}

void FinisherUI::Init()
{
	m_isVisible = true;
}

void FinisherUI::Update(Input& input)
{
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle)return;
	const auto& screen = battle->GetFinisherScreen();
	const float goal = screen.isPlaying ? 1.0f : 0.0f;
	m_barRate += (goal > m_barRate ? 1.0f : -1.0f) * kBarSpeed;
	if (m_barRate < 0.0f)m_barRate = 0.0f;
	if (m_barRate > 1.0f)m_barRate = 1.0f;
	m_whiteRate = screen.whiteRate;
	m_slashRate = screen.slashRate;
}

void FinisherUI::Draw()
{
	const int width = Game::GetScreenWidth();
	const int height = Game::GetScreenHeight();

	if (m_slashRate > 0.0f)
	{
		const float halfWidth = height * kSlashWidthRate * m_slashRate * m_slashRate;
		const float centerX = width * 0.5f;
		const float centerY = height * 0.5f;
		const float length = static_cast<float>(width);
		const float dirX = cosf(kSlashAngle);
		const float dirY = sinf(kSlashAngle);
		const float x1 = centerX - dirX * length;
		const float y1 = centerY - dirY * length;
		const float x2 = centerX + dirX * length;
		const float y2 = centerY + dirY * length;
		const float nx = -dirY * halfWidth;
		const float ny = dirX * halfWidth;
		SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
		DrawTriangleAA(x1, y1, x2 + nx, y2 + ny, x2 - nx, y2 - ny, 0xffffff, TRUE);
		DrawTriangleAA(x2, y2, x1 + nx, y1 + ny, x1 - nx, y1 - ny, 0xffffff, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	if (m_whiteRate > 0.0f)
	{
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * m_whiteRate));
		DrawBox(0, 0, width, height, 0xffffff, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	if (m_barRate > 0.0f)
	{
		const int barHeight = static_cast<int>(height * kBarHeightRate * m_barRate);
		DrawBox(0, 0, width, barHeight, 0x000000, TRUE);
		DrawBox(0, height - barHeight, width, height, 0x000000, TRUE);
	}
}
