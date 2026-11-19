#include "EndingCamera.h"
#include "../CameraManager.h"
#include "Character/Player/Player.h"
#include <cmath>

namespace
{
	constexpr float kDegToRad = DX_PI_F / 180.0f;
	constexpr float kBlendDuration = 24.0f;//前のカメラから寄るフレーム数
	constexpr float kBlendEasingPower = 2.0f;
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
