#include "FinisherUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"

void FinisherUI::Init()
{
	m_isVisible = true;
}

void FinisherUI::Update()
{
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle)return;
	m_whiteRate = battle->GetFinisherScreen().whiteRate;
}

void FinisherUI::Draw()
{
	if (m_whiteRate <= 0.0f)return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * m_whiteRate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), 0xffffff, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
