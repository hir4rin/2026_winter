#include "KessatsuUI.h"
#include "DxLib.h"
#include "../Game.h"
#include "../System.h"
#include "../BattleManager.h"
#include "../Camera/CameraManager.h"
#include "../Camera/CameraState/CameraStateBase.h"

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

	//ラストヒットイベント用の「血」の位置
	constexpr float kLastHitChiX = 220.0f;
	constexpr float kLastHitChiY = 220.0f;
	//ラストヒットイベント用の「殺」の位置(「血」からのずれ)
	constexpr float kLastHitSatsuOffsetX = 200.0f;
	constexpr float kLastHitSatsuOffsetY = 170.0f;
}

KessatsuUI::KessatsuUI(std::weak_ptr<CameraManager> cameraManager) :
	m_cameraManager(cameraManager)
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

	//ラストヒットイベント中は配置違いで表示する
	if (IsLastHitEvent())
	{
		DrawLastHit();
	}
	else
	{
		DrawUlt();
	}
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

void KessatsuUI::DrawLastHit()
{
	constexpr float kScreenW = static_cast<float>(Game::kScreenWidth);
	constexpr float kScreenH = static_cast<float>(Game::kScreenHeight);

	//「血」:左上
	DrawDesignGraph(kLastHitChiX, kLastHitChiY, kBloodScale, kBloodRotation, m_bloodHandle);
	//「殺」:血の少し右下
	DrawDesignGraph(kLastHitChiX + kLastHitSatsuOffsetX, kLastHitChiY + kLastHitSatsuOffsetY, kSatsuScale, 0.0f, m_satsuHandle);
	//液体:右下寄り
	DrawDesignGraph(kScreenW - kScreenW / 15.0f, kScreenH - 150.0f, kSplashScale, kSplashRotation, m_splashHandle);
}

bool KessatsuUI::IsLastHitEvent() const
{
	auto cameraManager = m_cameraManager.lock();
	if (!cameraManager) return false;
	auto camera = cameraManager->GetActiveCamera();
	if (!camera) return false;

	const auto type = camera->GetCameraType();
	return type == CameraStateBase::Type::FinishingFirstCamera
		|| type == CameraStateBase::Type::FinishingSecondCamera;
}

void KessatsuUI::DrawDesignGraph(float x, float y, float scale, float angle, int handle)
{
	//元はレンダーターゲット(設計解像度)に描いてから画面全体に引き伸ばしていたので、
	//ここでは座標と拡大率を実解像度に合わせて直接描画する
	DrawRotaGraphF(Game::ScaleX(x), Game::ScaleY(y), scale * Game::GetScale(), angle, handle, TRUE);
}
