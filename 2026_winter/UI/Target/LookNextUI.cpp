#include "LookNextUI.h"
#include <cmath>
#include <algorithm>
#include "DxLib.h"
#include "Main/Game.h"
#include "Camera/CameraManager.h"
#include "Camera/CameraState/CameraStateBase.h"

namespace
{
	//色
	constexpr int kSilverR = 232;
	constexpr int kSilverG = 236;
	constexpr int kSilverB = 242;
	constexpr int kRedR = 220;
	constexpr int kRedG = 58;
	constexpr int kRedB = 72;

	//入り・消え
	constexpr float kInFrame = 12.0f;
	constexpr float kInAlphaFrame = 5.0f;
	constexpr float kPopScale = 1.6f;//出始めの大きさ
	constexpr float kFadeFrame = 10.0f;

	//紋(設計解像度)
	constexpr float kRadius = 22.0f;//外の円
	constexpr float kRingThick = 2.0f;
	constexpr float kGlowThick = 7.0f;
	constexpr int kGlowAlpha = 80;
	constexpr float kDiamondRate = 0.62f;//菱形の大きさ(外の円に対する割合)
	constexpr float kDiamondThick = 2.5f;
	constexpr float kDotRate = 0.14f;//真ん中の点
	constexpr float kTickStartRate = 1.12f;//目盛り
	constexpr float kTickEndRate = 1.38f;
	constexpr float kTickThick = 2.0f;
	constexpr float kSpinSpeed = 0.01f;//菱形の回る速さ
	constexpr int kCirclePosNum = 48;
	constexpr float kEdgeThick = 2.0f;//黒いふち(足す太さ)
	constexpr int kEdgeAlpha = 140;
}

LookNextUI::LookNextUI(std::weak_ptr<CameraManager> cameraManager) :
	m_cameraManager(cameraManager)
{
}

void LookNextUI::Init()
{
	m_isVisible = false;
}

void LookNextUI::Update()
{
	m_totalFrame++;

	if (IsLooking())
	{
		//消えている途中にまた見たらやり直す
		if (m_isFading || !m_isVisible) Hide();
		m_targetPos = m_cameraManager.lock()->GetActiveCamera()->GetTarget();
		m_frame++;
		m_isVisible = true;
		return;
	}

	if (!m_isVisible) return;

	//離したら消す
	m_isFading = true;
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame) Hide();
}

void LookNextUI::Draw()
{
	if (!m_isVisible) return;

	//カメラの後ろなら出さない(注視点はファーより遠いことがあるので、zではなく前向きで見る)
	const VECTOR toTarget = VSub(m_targetPos.ToDxLibVector(), GetCameraPosition());
	if (VDot(toTarget, GetCameraFrontVector()) <= 0.0f) return;

	const VECTOR screenPos = ConvWorldPosToScreenPos(m_targetPos.ToDxLibVector());

	//出始めは大きめから縮む
	const float scale = Game::GetScale() * (1.0f + (kPopScale - 1.0f) * (1.0f - GetInRate()));
	DrawCrest(screenPos.x, screenPos.y, scale, GetAlpha());

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

bool LookNextUI::IsLooking() const
{
	auto cameraManager = m_cameraManager.lock();
	if (!cameraManager) return false;
	auto camera = cameraManager->GetActiveCamera();
	return camera && camera->GetCameraType() == CameraStateBase::Type::LookingNextCamera;
}

void LookNextUI::Hide()
{
	m_frame = 0;
	m_isFading = false;
	m_fadeFrame = 0;
	m_isVisible = false;
}

float LookNextUI::GetInRate() const
{
	//だんだん遅くなる
	float rate = m_frame / kInFrame;
	if (rate > 1.0f) rate = 1.0f;
	return 1.0f - (1.0f - rate) * (1.0f - rate) * (1.0f - rate);
}

float LookNextUI::GetAlpha() const
{
	if (m_isFading) return 1.0f - m_fadeFrame / kFadeFrame;
	return (std::min)(1.0f, m_frame / kInAlphaFrame);
}

void LookNextUI::DrawCrest(float x, float y, float scale, float alpha)
{
	const unsigned int silver = GetColor(kSilverR, kSilverG, kSilverB);
	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	const unsigned int black = GetColor(0, 0, 0);
	const float radius = kRadius * scale;

	//外の円(黒いふち → 赤い光 → 銀)
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kEdgeAlpha * alpha));
	DrawCircleAA(x, y, radius, kCirclePosNum, black, FALSE, (kRingThick + kEdgeThick) * scale);
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kGlowAlpha * alpha));
	DrawCircleAA(x, y, radius, kCirclePosNum, red, FALSE, kGlowThick * scale);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	DrawCircleAA(x, y, radius, kCirclePosNum, silver, FALSE, kRingThick * scale);

	//回る菱形と目盛り
	const float spin = m_totalFrame * kSpinSpeed;
	const float diamond = radius * kDiamondRate;
	float px[4];
	float py[4];
	for (int i = 0; i < 4; i++)
	{
		const float a = spin + DX_PI_F * 0.5f * i;
		px[i] = x + std::cos(a) * diamond;
		py[i] = y + std::sin(a) * diamond;
	}
	for (int i = 0; i < 4; i++)
	{
		const int next = (i + 1) % 4;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kEdgeAlpha * alpha));
		DrawLineAA(px[i], py[i], px[next], py[next], black, (kDiamondThick + kEdgeThick) * scale);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
		DrawLineAA(px[i], py[i], px[next], py[next], red, kDiamondThick * scale);

		const float a = spin + DX_PI_F * 0.25f + DX_PI_F * 0.5f * i;
		const float sx = x + std::cos(a) * radius * kTickStartRate;
		const float sy = y + std::sin(a) * radius * kTickStartRate;
		const float ex = x + std::cos(a) * radius * kTickEndRate;
		const float ey = y + std::sin(a) * radius * kTickEndRate;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kEdgeAlpha * alpha));
		DrawLineAA(sx, sy, ex, ey, black, (kTickThick + kEdgeThick) * scale);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
		DrawLineAA(sx, sy, ex, ey, silver, kTickThick * scale);
	}

	//真ん中の点
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	DrawCircleAA(x, y, radius * kDotRate, kCirclePosNum / 4, red, TRUE);
}
