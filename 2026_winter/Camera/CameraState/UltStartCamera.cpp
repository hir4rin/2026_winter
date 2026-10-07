#include "UltStartCamera.h"
#include "../CameraManager.h"
#include "Player.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../System.h"
#include <cmath>

namespace
{
	constexpr float kBlendDuration = 15.0f;//このStateに入るときのブレンドフレーム数
	constexpr float kBlendEasingPower = 2.0f;//このStateに入るときのイージング指数
}

UltStartCamera::UltStartCamera(std::weak_ptr<CameraManager> owner) : CameraStateBase(owner)
{
}

UltStartCamera::~UltStartCamera()
{
}

void UltStartCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_pos = data.pos;
	m_target = data.target;
	m_goalPos = data.pos;
	m_goalTarget = data.target;
	ResetBlend(data.pos, data.target);
}

void UltStartCamera::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;

	FixCameraPos();

	if (IsBlending())
	{
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	else
	{
		m_pos = m_goalPos;
		m_target = m_goalTarget;
	}

	m_timer += 1.0f * System::GetInstance().GetTimeScale();
}

void UltStartCamera::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void UltStartCamera::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//TODO:カメラ位置・注視点を計算する
}

void UltStartCamera::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting UltStartCamera::GetBlendSetting() const
{
	return BlendSetting{
		.mode = BlendSetting::Mode::Lerp,
		.duration = kBlendDuration,
		.easingMode = EasingMode::EaseOut,
		.easingPower = kBlendEasingPower
	};
}
