#include "KessatsuUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace
{
	//画像のパス
	const char* const kBloodPath = "data/UI/bloodUI/blood_UI.png";
	const char* const kSplashPath = "data/UI/bloodUI/splash2_UI.png";
	const char* const kSatsuPath = "data/UI/bloodUI/satu_UI.png";

	constexpr float kBloodScale = 0.8f;//「血」の拡大率
	constexpr float kBloodRotation = -DX_PI_F / 12;//「血」の回転角度
	constexpr float kSatsuScale = 0.5f;//「殺」の拡大率
	constexpr float kSplashScale = 1.0f;//液体の拡大率
	constexpr float kSplashRotation = DX_PI_F / 4;//液体の回転角度

	const char* const kBurstPathFormat = "data/UI/bloodUI/white_burst_";
	constexpr float kBurstScale = 0.7f;//白い爆発の拡大率
	constexpr int kBurstAngleUp = 30;//右上へ飛ばす角度(度)
	constexpr int kBurstAngleRange = 12;//そこからずらす最大(度)
	constexpr float kBurstStartRate = 0.45f;//出始めの大きさ
	constexpr float kBurstCreepRate = 0.08f;//止まっている間に広がる分
	//実フレーム
	constexpr int kBurstGrowFrame = 4;
	constexpr int kBurstCreepFrame = 66;//少しずつ広がる長さ

	const char* const kSlashPath = "data/UI/bloodUI/slash_streak.png";
	constexpr float kSlashImageAnchorX = 120.0f;//画像のこの位置を爆発の中心に合わせる
	constexpr float kSlashImageCenterY = 48.0f;
	constexpr float kSlashMainLength = 0.85f;//一番長い線の長さの倍率
	constexpr float kSlashMainThick = 0.85f;//一番太い線の太さの倍率
	constexpr float kSlashSubThick = 0.5f;//2本目の太さの倍率
	constexpr float kSlashSubThickDown = 0.15f;//3本目で細くする分
	constexpr int kSlashSubAngleMin = 2;//2本目以降をずらす角度(度)
	constexpr int kSlashSubAngleMax = 7;
	constexpr int kSlashSubOffsetMin = 14;//2本目以降を横へずらす距離
	constexpr int kSlashSubOffsetMax = 34;
	//実フレーム
	constexpr int kSlashGrowFrame = 4;//伸びる
	constexpr int kSlashLineDelayFrame = 1;//線ごとの出るずれ

	const char* const kVignettePath = "data/UI/bloodUI/ink_vignette.png";
	//色変えと同じ速さ(実フレーム)
	constexpr float kVignetteFadeInFrame = 8.0f;
	constexpr float kVignetteFadeOutFrame = 15.0f;
	//敵が画面の外でも見えるように、この範囲に収める(設計解像度)
	constexpr float kBurstMinX = 260.0f;
	constexpr float kBurstMaxX = 1020.0f;
	constexpr float kBurstMinY = 180.0f;
	constexpr float kBurstMaxY = 540.0f;
}

KessatsuUI::KessatsuUI()
{
}

KessatsuUI::~KessatsuUI()
{
	DeleteGraph(m_bloodHandle);
	DeleteGraph(m_splashHandle);
	DeleteGraph(m_satsuHandle);
	for (int handle : m_burstHandles) DeleteGraph(handle);
	DeleteGraph(m_vignetteHandle);
	DeleteGraph(m_slashHandle);
}

void KessatsuUI::Init()
{
	m_bloodHandle = LoadGraph(kBloodPath);
	m_splashHandle = LoadGraph(kSplashPath);
	m_satsuHandle = LoadGraph(kSatsuPath);
	for (int i = 0; i < kBurstImageNum; ++i)
	{
		m_burstHandles[i] = LoadGraph((kBurstPathFormat + std::to_string(i) + ".png").c_str());
	}
	m_vignetteHandle = LoadGraph(kVignettePath);
	m_slashHandle = LoadGraph(kSlashPath);
	m_isVisible = false;
}

void KessatsuUI::Update()
{
	//必殺技中だけ表示する
	auto battleMgr = System::GetInstance().GetBattleMgr();
	m_isVisible = battleMgr && battleMgr->GetIsUltimating();

	if (m_isVisible && !m_wasUltimating) StartBurst();
	m_wasUltimating = m_isVisible;

	const float vignetteAdd = m_isVisible ? 1.0f / kVignetteFadeInFrame : -1.0f / kVignetteFadeOutFrame;
	m_vignetteRate = std::clamp(m_vignetteRate + vignetteAdd, 0.0f, 1.0f);

	if (m_burstFrame >= 0) ++m_burstFrame;
	//ビネットが消えきったら爆発も終わり
	if (!m_isVisible && m_vignetteRate <= 0.0f) m_burstFrame = -1;
}

void KessatsuUI::Draw()
{
	//文字と爆発が暗くならないように先に描く
	DrawVignette();
	DrawBurst();
	DrawSlash();

	if (!m_isVisible && m_vignetteRate <= 0.0f) return;

	//演出が終わったらビネットと一緒に消す
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, GetFadeAlpha());
	DrawUlt();
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

int KessatsuUI::GetFadeAlpha() const
{
	return m_isVisible ? 255 : static_cast<int>(255 * m_vignetteRate);
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

void KessatsuUI::DrawVignette()
{
	if (m_vignetteRate <= 0.0f) return;

	//黒をアルファで重ねる(乗算と同じ結果で、フェードできる)
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(120 * m_vignetteRate));
	DrawExtendGraphF(0.0f, 0.0f, static_cast<float>(Game::GetScreenWidth()), static_cast<float>(Game::GetScreenHeight()), m_vignetteHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

void KessatsuUI::StartBurst()
{
	m_burstFrame = 0;
	m_burstIndex = GetRand(kBurstImageNum - 1);
	//画像は右へ飛ぶ形なので、右上へ傾けて出す
	m_burstAngle = (GetRand(kBurstAngleRange * 2) - kBurstAngleRange - kBurstAngleUp) * DX_PI_F / 180.0f;
	m_burstX = Game::ScaleX(Game::kScreenWidth * 0.5f);
	m_burstY = Game::ScaleY(Game::kScreenHeight * 0.5f);
	StartSlash();
}

void KessatsuUI::StartSlash()
{
	for (int i = 0; i < kSlashLineNum; ++i)
	{
		SlashLine& line = m_slashLines[i];
		line.isFlip = GetRand(1) == 0;
		if (i == 0)
		{
			line.angle = 0.0f;
			line.sideOffset = 0.0f;
			line.length = kSlashMainLength;
			line.thick = kSlashMainThick;
			continue;
		}
		const float side = GetRand(1) == 0 ? 1.0f : -1.0f;
		line.angle = side * (kSlashSubAngleMin + GetRand(kSlashSubAngleMax - kSlashSubAngleMin)) * DX_PI_F / 180.0f;
		line.sideOffset = side * (kSlashSubOffsetMin + GetRand(kSlashSubOffsetMax - kSlashSubOffsetMin));
		line.length = kSlashMainLength * (0.45f + GetRand(30) / 100.0f);
		line.thick = kSlashSubThick - kSlashSubThickDown * (i - 1);
	}
}

void KessatsuUI::DrawSlash()
{
	if (m_burstFrame < 0) return;

	const float scale = Game::GetScale();
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	for (int i = 0; i < kSlashLineNum; ++i)
	{
		const int frame = m_burstFrame - i * kSlashLineDelayFrame;
		if (frame < 0) continue;

		//一気に伸びて止まる
		const float growT = 1.0f - (std::min)(1.0f, static_cast<float>(frame) / kSlashGrowFrame);
		const float grow = 1.0f - growT * growT;
		if (grow <= 0.0f) continue;

		const SlashLine& line = m_slashLines[i];
		const float angle = m_burstAngle + line.angle;
		const float x = m_burstX - sinf(angle) * line.sideOffset * scale;
		const float y = m_burstY + cosf(angle) * line.sideOffset * scale;
		//演出が終わったらビネットと一緒に消す
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, GetFadeAlpha());
		DrawRotaGraph3F(x, y, kSlashImageAnchorX, kSlashImageCenterY, line.length * grow * scale, line.thick * scale, angle, m_slashHandle, TRUE, FALSE, line.isFlip);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

void KessatsuUI::DrawBurst()
{
	if (m_burstFrame < 0) return;

	//カメラの後ろにあるときは前の位置のまま
	const auto battleMgr = System::GetInstance().GetBattleMgr();
	const VECTOR screen = ConvWorldPosToScreenPos(battleMgr->GetUltBurstPos().ToDxLibVector());
	if (screen.z > 0.0f && screen.z < 1.0f)
	{
		m_burstX = std::clamp(screen.x, Game::ScaleX(kBurstMinX), Game::ScaleX(kBurstMaxX));
		m_burstY = std::clamp(screen.y, Game::ScaleY(kBurstMinY), Game::ScaleY(kBurstMaxY));
	}

	float rate = 1.0f;
	const int alpha = GetFadeAlpha();
	if (m_burstFrame < kBurstGrowFrame)
	{
		//一気に広がって止まる
		const float t = 1.0f - static_cast<float>(m_burstFrame) / kBurstGrowFrame;
		rate = 1.0f - (1.0f - kBurstStartRate) * t * t;
	}
	else
	{
		const int afterGrow = m_burstFrame - kBurstGrowFrame;
		rate = 1.0f + kBurstCreepRate * (std::min)(1.0f, static_cast<float>(afterGrow) / kBurstCreepFrame);
	}

	SetDrawMode(DX_DRAWMODE_BILINEAR);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawRotaGraphF(m_burstX, m_burstY, kBurstScale * rate * Game::GetScale(), m_burstAngle, m_burstHandles[m_burstIndex], TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

void KessatsuUI::DrawDesignGraph(float x, float y, float scale, float angle, int handle)
{
	//元はレンダーターゲット(設計解像度)に描いてから画面全体に引き伸ばしていたので、
	//ここでは座標と拡大率を実解像度に合わせて直接描画する
	DrawRotaGraphF(Game::ScaleX(x), Game::ScaleY(y), scale * Game::GetScale(), angle, handle, TRUE);
}
