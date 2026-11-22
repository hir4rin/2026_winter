#include "EndingCamera.h"
#include "../CameraManager.h"
#include "Character/Player/Player.h"
#include <cmath>

namespace
{
	constexpr float kDegToRad = DX_PI_F / 180.0f;
	constexpr float kBlendDuration = 24.0f;//前のカメラから寄るフレーム数
	constexpr float kBlendEasingPower = 2.0f;
	constexpr float kHoldTargetHeight = 80.0f;//止まっているときに見る高さ
	constexpr float kHoldFollowRate = 0.25f;//止まっているときの注視点の追いかけ方
	constexpr float kHoldFov = 55.0f;//飛び降りるときの視野角
	constexpr float kHoldFovRate = 0.08f;//視野角を寄せる割合(1フレーム)
	constexpr float kModeSideSpeed = 1.8f;//横の動きを高さより早く終える倍率
	constexpr float kRiseFollowRate = 0.06f;//縦穴を登るとき、プレイヤーの高さに寄せる割合(蹴るたびの上下をならす)
	constexpr float kRiseBlendFrame = 40.0f;//前のカメラから移る長さ
}

EndingCamera::EndingCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

EndingCamera::~EndingCamera()
{
}

void EndingCamera::Enter(CameraData data)
{
	m_pos = data.pos;
	m_target = data.target;
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	ResetBlend(data.pos, data.target);
}

void EndingCamera::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	m_shotFrame += 1.0f;//実フレームで進める
	const Shot shot = CurrentShot();
	const Vector3 foot = player->GetRigidBody().GetPos();
	const Vector3 up = Vector3(0.0f, 1.0f, 0.0f);
	if (m_mode == Mode::Rise)
	{
		//横の位置は縦穴の線に固定し、高さだけなめらかにプレイヤーについていく(左右に蹴っても揺れない)
		m_modeFrame += 1.0f;
		auto smooth = [](float x) { return x * x * (3.0f - 2.0f * x); };
		const float t = smooth((m_modeLength > 0.0f) ? std::clamp(m_modeFrame / m_modeLength, 0.0f, 1.0f) : 1.0f);
		m_riseY = std::lerp(m_riseY, foot.y, kRiseFollowRate);
		const float dist = std::lerp(m_riseStartDist, m_riseEndDist, t);
		const float upOffset = std::lerp(m_riseStartUp, m_riseEndUp, t);
		const Vector3 goalPos = Vector3(m_riseCenter.x, m_riseY + upOffset, m_riseCenter.z) + m_riseBack * dist;
		const Vector3 goalTarget = Vector3(m_riseCenter.x, m_riseY + kHoldTargetHeight, m_riseCenter.z);
		//切り替えた直後は、前のカメラからなめらかに移る
		const float blend = smooth(std::clamp(m_modeFrame / kRiseBlendFrame, 0.0f, 1.0f));
		m_pos = Vector3::Lerp(m_modeFromPos, goalPos, blend);
		m_target = Vector3::Lerp(m_modeFromTarget, goalTarget, blend);
		m_goalFov = kHoldFov * kDegToRad;
		m_fov = std::lerp(m_fov, m_goalFov, kHoldFovRate);
		return;
	}
	if (m_mode != Mode::Shot)
	{
		//切り替えた時の位置からなめらかに動かし、注視点はプレイヤーを追いかける
		m_modeFrame += 1.0f;
		const float t = (m_modeLength > 0.0f) ? std::clamp(m_modeFrame / m_modeLength, 0.0f, 1.0f) : 1.0f;
		auto smooth = [](float x) { return x * x * (3.0f - 2.0f * x); };
		//車両の上の物を通り抜けないように、上がるときは先に上へ、下がるときは先に横へ動かす
		const Vector3 goal = (m_mode == Mode::Hold) ? m_holdPos : foot + m_followOffset;
		const float fast = smooth((std::min)(1.0f, t * kModeSideSpeed));
		const float slow = smooth(t * t);
		const bool isRising = goal.y > m_modeFromPos.y;
		const float th = isRising ? slow : fast;
		const float tv = isRising ? fast : slow;
		m_pos = Vector3(std::lerp(m_modeFromPos.x, goal.x, th), std::lerp(m_modeFromPos.y, goal.y, tv), std::lerp(m_modeFromPos.z, goal.z, th));
		//カット(長さ0)のときは注視点もすぐ合わせる
		m_target = (m_modeLength <= 0.0f && m_modeFrame <= 1.0f) ? foot + up * kHoldTargetHeight : Vector3::Lerp(m_target, foot + up * kHoldTargetHeight, kHoldFollowRate);
		m_goalFov = kHoldFov * kDegToRad;
		m_fov = std::lerp(m_fov, m_goalFov, kHoldFovRate);
		return;
	}
	m_goalPos = foot - m_forward * shot.back + up * shot.height;
	m_goalTarget = foot + m_forward * shot.targetAhead + up * shot.targetHeight;
	m_goalFov = shot.fov * kDegToRad;

	if (IsBlending())
	{
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	else
	{
		m_pos = m_goalPos;
		m_target = m_goalTarget;
		m_fov = m_goalFov;
	}
}

void EndingCamera::Exit()
{
}

void EndingCamera::HoldAt(const Vector3& pos, float frame)
{
	m_mode = Mode::Hold;
	m_holdPos = pos;
	m_modeFromPos = m_pos;
	m_modeFrame = 0.0f;
	m_modeLength = frame;
}

void EndingCamera::FollowAt(const Vector3& offset, float frame)
{
	m_mode = Mode::Follow;
	m_followOffset = offset;
	m_modeFromPos = m_pos;
	m_modeFrame = 0.0f;
	m_modeLength = frame;
}

void EndingCamera::RiseAlong(const Vector3& center, const Vector3& backDir, float startDist, float endDist, float startUp, float endUp, float frame)
{
	m_mode = Mode::Rise;
	m_riseCenter = center;
	m_riseBack = backDir;
	m_riseStartDist = startDist;
	m_riseEndDist = endDist;
	m_riseStartUp = startUp;
	m_riseEndUp = endUp;
	m_modeFromPos = m_pos;
	m_modeFromTarget = m_target;
	m_modeFrame = 0.0f;
	m_modeLength = frame;
	if (auto cameraManager = m_owner.lock())
	{
		if (auto player = cameraManager->GetContext()->m_player.lock()) m_riseY = player->GetRigidBody().GetPos().y;
	}
}

void EndingCamera::MoveTo(const Shot& shot, float frame)
{
	m_fromShot = CurrentShot();
	m_toShot = shot;
	m_shotFrame = 0.0f;
	m_shotLength = (frame > 0.0f) ? frame : 0.0f;
	if (frame <= 0.0f) m_fromShot = shot;
}

EndingCamera::Shot EndingCamera::CurrentShot() const
{
	float t = (m_shotLength > 0.0f) ? std::clamp(m_shotFrame / m_shotLength, 0.0f, 1.0f) : 1.0f;
	t = t * t * (3.0f - 2.0f * t);//ゆっくり始まってゆっくり止まる
	return {
		std::lerp(m_fromShot.back, m_toShot.back, t),
		std::lerp(m_fromShot.height, m_toShot.height, t),
		std::lerp(m_fromShot.targetAhead, m_toShot.targetAhead, t),
		std::lerp(m_fromShot.targetHeight, m_toShot.targetHeight, t),
		std::lerp(m_fromShot.fov, m_toShot.fov, t),
	};
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting EndingCamera::GetBlendSetting() const
{
	return BlendSetting{
		.mode = BlendSetting::Mode::Lerp,
		.duration = kBlendDuration,
		.easingMode = EasingMode::EaseOut,
		.easingPower = kBlendEasingPower,
	};
}
