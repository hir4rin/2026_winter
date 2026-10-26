#include "KessatsuUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"

namespace
{
	//画像のパス
	const char* const kBloodPath = "data/UI/blood_UI.png";
	const char* const kSplashPath = "data/UI/splash2_UI.png";
	const char* const kSatsuPath = "data/UI/satu_UI.png";

	constexpr float kBloodScale = 0.8f;//「血」の拡大率
	constexpr float kBloodRotation = -DX_PI_F / 12;//「血」の回転角度
	constexpr float kSatsuScale = 0.5f;//「殺」の拡大率
	constexpr float kSplashScale = 1.0f;//液体の拡大率
	constexpr float kSplashRotation = DX_PI_F / 4;//液体の回転角度
}

KessatsuUI::KessatsuUI()
{
}

KessatsuUI::~KessatsuUI()
{
	DeleteGraph(m_bloodHandle);
	DeleteGraph(m_splashHandle);
	DeleteGraph(m_satsuHandle);
}

void KessatsuUI::Init()
{
	m_bloodHandle = LoadGraph(kBloodPath);
	m_splashHandle = LoadGraph(kSplashPath);
	m_satsuHandle = LoadGraph(kSatsuPath);
	m_isVisible = false;
}

void KessatsuUI::Update(Input& input)
{
	//必殺技中だけ表示する
	auto battleMgr = System::GetInstance().GetBattleMgr();
	m_isVisible = battleMgr && battleMgr->GetIsUltimating();
}

void KessatsuUI::Draw()
{
	if (!m_isVisible) return;

	DrawUlt();
}

void KessatsuUI::DrawUlt()
{
	constexpr float kScreenW = static_cast<float>(Game::kScreenWidth);
	constexpr float kScreenH = static_cast<float>(Game::kScreenHeight);

	//「血」:右上寄り
	DrawDesignGraph(kScreenW - 200.0f, kScreenH / 4.0f, kBloodScale, kBloodRotation, m_bloodHandle);
	//液体:左下寄り
	DrawDesignGraph(kScreenW / 15.0f, kScreenH - 150.0f, kSplashScale, kSplashRotation, m_splashHandle);
	//「殺」:右下寄り
	DrawDesignGraph(kScreenW * 3.0f / 5.0f, kScreenH - 200.0f, kSatsuScale, 0.0f, m_satsuHandle);
}

void KessatsuUI::DrawDesignGraph(float x, float y, float scale, float angle, int handle)
{
	//元はレンダーターゲット(設計解像度)に描いてから画面全体に引き伸ばしていたので、
	//ここでは座標と拡大率を実解像度に合わせて直接描画する
	DrawRotaGraphF(Game::ScaleX(x), Game::ScaleY(y), scale * Game::GetScale(), angle, handle, TRUE);
}
