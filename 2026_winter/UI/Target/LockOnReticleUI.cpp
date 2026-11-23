#include "LockOnReticleUI.h"
#include <cmath>
#include <algorithm>
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Camera/LockOnManager.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"

namespace
{
	//色
	constexpr int kCyanR = 57;
	constexpr int kCyanG = 243;
	constexpr int kCyanB = 255;
	constexpr int kRedR = 230;
	constexpr int kRedG = 32;
	constexpr int kRedB = 56;

	//位置
	constexpr float kCenterRate = 0.6f;//体のカプセルの足〜頭のどこに出すか
	constexpr float kBaseBodyRadius = 40.0f;//この太さの敵で大きさ1
	constexpr float kMaxSizeScale = 2.0f;

	//距離で大きさを変える
	constexpr float kNearDistance = 500.0f;
	constexpr float kFarDistance = 5000.0f;
	constexpr float kNearScale = 1.0f;
	constexpr float kFarScale = 0.55f;

	//入り・消え
	constexpr float kInFrame = 10.0f;
	constexpr float kInAlphaFrame = 4.0f;
	constexpr float kPopScale = 1.35f;//出始めの大きさ
	constexpr float kInSpinAngle = DX_PI_F;//入りで余分に回す
	constexpr float kFadeFrame = 8.0f;

	//外の弧
	constexpr float kArcRadius = 60.0f;
	constexpr float kArcThick = 2.2f;
	constexpr float kArcGlowThick = 7.0f;
	constexpr int kArcGlowAlpha = 70;
	constexpr float kArcSpinSpeed = 0.012f;
	constexpr int kArcDivNum = 64;//一周の分割数
	constexpr int kArcNum = 3;
	constexpr float kArcStart[kArcNum] = { -80.0f, 40.0f, 150.0f };//度
	constexpr float kArcRange[kArcNum] = { 70.0f, 40.0f, 80.0f };

	//内の点線
	constexpr float kDotRadius = 53.0f;
	constexpr int kDotNum = 40;
	constexpr float kDotAngle = 2.5f;//度
	constexpr float kDotThick = 1.4f;
	constexpr int kDotAlpha = 150;
	constexpr float kDotSpinSpeed = 0.006f;

	//走査線
	constexpr float kScanHalfWidth = 46.0f;
	constexpr float kScanRange = 46.0f;
	constexpr float kScanSpeed = 0.05f;
	constexpr int kScanDivNum = 12;
	constexpr float kScanThick = 1.4f;
	constexpr float kScanGlowThick = 5.0f;
	constexpr int kScanAlpha = 220;
	constexpr int kScanGlowAlpha = 50;

	//赤い角
	constexpr float kLockDelay = 3.0f;
	constexpr float kLockFrame = 6.0f;
	constexpr float kLockPopScale = 1.7f;
	constexpr float kBoxHalf = 28.0f;
	constexpr float kCornerLength = 11.0f;
	constexpr float kBoxThick = 2.4f;
	constexpr float kBoxGlowThick = 7.0f;
	constexpr int kBoxGlowAlpha = 90;
	constexpr float kEdgeThick = 2.0f;//黒いふち(足す太さ)
	constexpr int kEdgeAlpha = 150;
	constexpr float kCenterDotRadius = 2.6f;
	constexpr int kCenterDotPosNum = 12;

	//グリッチ
	constexpr int kGlitchInterval = 157;
	constexpr int kGlitchFrame = 2;
	constexpr float kGlitchMove = 3.0f;
}

LockOnReticleUI::LockOnReticleUI(std::weak_ptr<Player> player) :
	m_player(player)
{
}

void LockOnReticleUI::Init()
{
	m_isVisible = false;
}

void LockOnReticleUI::Update()
{
	m_totalFrame++;

	auto target = FindTarget();
	if (target)
	{
		//対象が変わった・消えている途中にまたロックしたらやり直す
		if (target != m_target.lock() || m_isFading)
		{
			Hide();
			m_target = target;
		}
		m_frame++;
		m_isVisible = true;
		return;
	}

	if (!m_isVisible) return;

	//外れたら消す
	m_isFading = true;
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame) Hide();
}

void LockOnReticleUI::Draw()
{
	if (!m_isVisible) return;

	//消えている間は位置を止める
	if (!m_isFading)
	{
		auto target = m_target.lock();
		if (!target) return;

		const EnemyStatus& status = target->GetStatus();
		const float height = status.bodyColOffsetY + (status.bodyColHeight - status.bodyColOffsetY) * kCenterRate;
		const Vector3 uiPos = target->GetRigidBody().GetPos() + Vector3(0.0f, height, 0.0f);
		const VECTOR screenPos = ConvWorldPosToScreenPos(uiPos.ToDxLibVector());
		//カメラの後ろなら出さない
		m_isOnScreen = screenPos.z > 0.0f && screenPos.z < 1.0f;
		if (!m_isOnScreen) return;

		//遠いほど小さく
		const VECTOR cameraPos = GetCameraPosition();
		const float distance = VSize(VSub(uiPos.ToDxLibVector(), cameraPos));
		const float t = std::clamp((distance - kNearDistance) / (kFarDistance - kNearDistance), 0.0f, 1.0f);
		const float distScale = kNearScale + (kFarScale - kNearScale) * t;

		m_drawX = screenPos.x;
		m_drawY = screenPos.y;
		m_drawScale = Game::GetScale() * distScale * GetSizeScale(target);
	}
	if (!m_isOnScreen) return;

	const float inRate = GetInRate();
	const float scale = m_drawScale * (1.0f + (kPopScale - 1.0f) * (1.0f - inRate));
	const float alpha = GetAlpha();

	//たまに映像がずれる
	float x = m_drawX;
	const float y = m_drawY;
	if (m_totalFrame % kGlitchInterval < kGlitchFrame)
	{
		const float dir = (m_totalFrame / kGlitchInterval) % 2 == 0 ? 1.0f : -1.0f;
		x += kGlitchMove * Game::GetScale() * dir;
	}

	DrawDots(x, y, scale, alpha);
	DrawArcs(x, y, scale, alpha);
	DrawScanLine(x, y, scale, alpha);
	DrawLockBox(x, y, m_drawScale, alpha);

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

std::shared_ptr<EnemyBase> LockOnReticleUI::FindTarget() const
{
	auto player = m_player.lock();
	if (!player || !player->IsLockOn()) return nullptr;

	//演出中・ガードブレイクのチャンス中は出さない
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle || battle->GetIsUltimating() || battle->GetIsEventPlaying()) return nullptr;
	if (battle->GetFinisherScreen().isPlaying || battle->GetEndingScreen().isPlaying) return nullptr;
	if (player->GetGuardBreakChanceState()) return nullptr;

	auto lockOn = player->GetLockOnManager();
	if (!lockOn) return nullptr;
	return lockOn->GetLockTarget();
}

void LockOnReticleUI::Hide()
{
	m_target.reset();
	m_frame = 0;
	m_isFading = false;
	m_fadeFrame = 0;
	m_isOnScreen = false;
	m_isVisible = false;
}

float LockOnReticleUI::GetInRate() const
{
	//だんだん遅くなる
	float rate = m_frame / kInFrame;
	if (rate > 1.0f) rate = 1.0f;
	return 1.0f - (1.0f - rate) * (1.0f - rate);
}

float LockOnReticleUI::GetAlpha() const
{
	if (m_isFading) return 1.0f - m_fadeFrame / kFadeFrame;
	return (std::min)(1.0f, m_frame / kInAlphaFrame);
}

float LockOnReticleUI::GetSizeScale(const std::shared_ptr<EnemyBase>& target) const
{
	const float rate = std::sqrt(target->GetStatus().bodyColRadius / kBaseBodyRadius);
	return std::clamp(rate, 1.0f, kMaxSizeScale);
}

void LockOnReticleUI::DrawArcs(float x, float y, float scale, float alpha)
{
	//入りは速く回って止まる
	const float spin = m_totalFrame * kArcSpinSpeed + (1.0f - GetInRate()) * kInSpinAngle;
	const float radius = kArcRadius * scale;
	const unsigned int color = GetColor(kCyanR, kCyanG, kCyanB);

	for (int i = 0; i < kArcNum; i++)
	{
		const float start = kArcStart[i] * DX_PI_F / 180.0f + spin;
		const float range = kArcRange[i] * DX_PI_F / 180.0f;
		//光
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kArcGlowAlpha * alpha));
		DrawArc(x, y, radius, start, range, kArcGlowThick * scale, color);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
		DrawArc(x, y, radius, start, range, kArcThick * scale, color);
	}
}

void LockOnReticleUI::DrawDots(float x, float y, float scale, float alpha)
{
	//外の弧と逆に回す
	const float spin = -m_totalFrame * kDotSpinSpeed;
	const float radius = kDotRadius * scale;
	const float dotRange = kDotAngle * DX_PI_F / 180.0f;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kDotAlpha * alpha));
	for (int i = 0; i < kDotNum; i++)
	{
		const float start = spin + DX_TWO_PI_F * i / kDotNum;
		DrawArc(x, y, radius, start, dotRange, kDotThick * scale, GetColor(kCyanR, kCyanG, kCyanB));
	}
}

void LockOnReticleUI::DrawScanLine(float x, float y, float scale, float alpha)
{
	//上下に動かす
	const float lineY = y + std::sin(m_totalFrame * kScanSpeed) * kScanRange * scale;
	const float halfWidth = kScanHalfWidth * scale;
	const unsigned int color = GetColor(kCyanR, kCyanG, kCyanB);

	//端ほど薄く
	for (int i = 0; i < kScanDivNum; i++)
	{
		const float t0 = static_cast<float>(i) / kScanDivNum;
		const float t1 = static_cast<float>(i + 1) / kScanDivNum;
		const float center = (t0 + t1) - 1.0f;
		const float fade = 1.0f - center * center;
		const float x0 = x - halfWidth + halfWidth * 2.0f * t0;
		const float x1 = x - halfWidth + halfWidth * 2.0f * t1;

		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kScanGlowAlpha * fade * alpha));
		DrawLineAA(x0, lineY, x1, lineY, color, kScanGlowThick * scale);
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kScanAlpha * fade * alpha));
		DrawLineAA(x0, lineY, x1, lineY, color, kScanThick * scale);
	}
}

void LockOnReticleUI::DrawLockBox(float x, float y, float scale, float alpha)
{
	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	const unsigned int black = GetColor(0, 0, 0);

	float rate = (m_frame - kLockDelay) / kLockFrame;
	if (m_isFading) rate = 1.0f;
	if (rate > 0.0f)
	{
		//外からパチッと寄せる
		if (rate > 1.0f) rate = 1.0f;
		const float ease = 1.0f - (1.0f - rate) * (1.0f - rate);
		const float boxScale = 1.0f + (kLockPopScale - 1.0f) * (1.0f - ease);
		const float half = kBoxHalf * scale * boxScale;
		const float length = kCornerLength * scale * boxScale;
		const float boxAlpha = alpha * rate;

		for (int i = 0; i < 4; i++)
		{
			const float sx = (i == 0 || i == 3) ? -1.0f : 1.0f;
			const float sy = (i < 2) ? -1.0f : 1.0f;
			const float cx = x + half * sx;
			const float cy = y + half * sy;
			const float hx = cx - length * sx;
			const float vy = cy - length * sy;

			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kEdgeAlpha * boxAlpha));
			DrawLineAA(cx, cy, hx, cy, black, (kBoxThick + kEdgeThick) * scale);
			DrawLineAA(cx, cy, cx, vy, black, (kBoxThick + kEdgeThick) * scale);
			SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kBoxGlowAlpha * boxAlpha));
			DrawLineAA(cx, cy, hx, cy, red, kBoxGlowThick * scale);
			DrawLineAA(cx, cy, cx, vy, red, kBoxGlowThick * scale);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * boxAlpha));
			DrawLineAA(cx, cy, hx, cy, red, kBoxThick * scale);
			DrawLineAA(cx, cy, cx, vy, red, kBoxThick * scale);
		}
	}

	//真ん中の点
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	DrawCircleAA(x, y, (kCenterDotRadius + kEdgeThick * 0.5f) * scale, kCenterDotPosNum, black, TRUE);
	DrawCircleAA(x, y, kCenterDotRadius * scale, kCenterDotPosNum, red, TRUE);
}

void LockOnReticleUI::DrawArc(float x, float y, float radius, float startAngle, float angleRange, float thick, unsigned int color)
{
	const int divNum = (std::max)(1, static_cast<int>(std::ceil(kArcDivNum * angleRange / DX_TWO_PI_F)));
	const float step = angleRange / divNum;
	float angle = startAngle;
	for (int i = 0; i < divNum; i++)
	{
		const float next = angle + step;
		DrawLineAA(x + std::cos(angle) * radius, y + std::sin(angle) * radius,
			x + std::cos(next) * radius, y + std::sin(next) * radius, color, thick);
		angle = next;
	}
}
