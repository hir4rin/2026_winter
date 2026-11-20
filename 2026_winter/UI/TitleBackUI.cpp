#include "TitleBackUI.h"
#include "DxLib.h"
#include "Main/Game.h"

namespace
{
	const char* const kVignettePath = "data/UI/Menu/vignette.png";
}

TitleBackUI::~TitleBackUI()
{
	DeleteGraph(m_vignetteHandle);
}

void TitleBackUI::Init()
{
	m_vignetteHandle = LoadGraph(kVignettePath);
	m_isVisible = true;
}

void TitleBackUI::Update()
{
}

void TitleBackUI::Draw()
{
	if (!m_isVisible)return;
	const int w = Game::GetScreenWidth();
	const int h = Game::GetScreenHeight();
	//背景の街(TitleCity)の上に周辺減光だけ掛ける
	SetDrawBlendMode(DX_BLENDMODE_MUL, 255);
	DrawExtendGraph(0, 0, w, h, m_vignetteHandle, FALSE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
