#include "PartBrokenBCameraStart.h"
#include "../../CameraManager.h"
#include "Player.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../System.h"
#include <cmath>

namespace
{
	constexpr float kBlendDuration = 30.0f;//このStateに入るときのブレンドフレーム数
	constexpr float kBlendEasingPower = 2.0f;//このStateに入るときのイージング指数
	constexpr float kStayFrame = 60.0f;//このStateに留まるフレーム数
	constexpr float kCameraDistance = 300.0f;//プレイヤーからカメラまでの水平距離
	constexpr float kRightAngleDeg = 45.0f;//真後ろから右へずらす角度(度)
	constexpr float kDegToRad = DX_PI_F / 180.0f;
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//カメラの高さ

	constexpr float kNextBlendDuration = 40.0f;//AssasinCameraStateへのSlerpのフレーム数

	constexpr float kCameraViewAngle = DX_PI_F / 3.0f;//カメラの視野角
	constexpr float kCameraNear = 10.0f;//ニアクリップ(0だとDxLibの深度計算が壊れて何も映らなくなる)
	constexpr float kCameraFar = 5000.0f;//ファークリップ(SceneMainと同じ値)
}

PartBrokenBCameraStart::PartBrokenBCameraStart(std::weak_ptr<CameraManager> owner):CameraStateBase(owner)
{
}

PartBrokenBCameraStart::~PartBrokenBCameraStart()
{
}

void PartBrokenBCameraStart::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_pos = data.pos;
	m_target = data.target;
	m_goalPos = data.pos;
	m_goalTarget = data.target;
	ResetBlend(data.pos, data.target);

	SetupCamera_Perspective(kCameraViewAngle);
	SetCameraNearFar(kCameraNear, kCameraFar);
}

void PartBrokenBCameraStart::Update()
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
	//if (m_timer > kStayFrame)
	//{
	//	cameraManager->ChangeState(std::make_shared<AssasinCameraState>(m_owner));
	//	return;
	//}
}

void PartBrokenBCameraStart::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void PartBrokenBCameraStart::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();

	//プレイヤーの向き(確殺対象がいれば敵の方向、いなければプレイヤーの前方)
	Vector3 forward = player->GetForward();
	if (auto enemy = player->GetPartBrokenTarget())
	{
		forward = enemy->GetRigidBody().GetPos() - playerPos;
	}
	forward.y = 0.0f;
	if (forward.Magnitude() < 0.0001f)
	{
		forward = Vector3(0.0f, 0.0f, 1.0f);
	}
	forward = forward.Normalize();

	//プレイヤーの右方向(左手系・Y上:前(sin,cos)に対して右(cos,-sin))
	Vector3 right = Vector3(forward.z, 0.0f, -forward.x);
	Vector3 back = forward * -1.0f;

	//真後ろから右へ kRightAngleDeg ずらした方向
	float rad = kRightAngleDeg * kDegToRad;
	Vector3 dir = (back * cosf(rad) + right * sinf(rad)).Normalize();

	m_goalPos = playerPos + dir * kCameraDistance + kCameraHeight;
	m_goalTarget = playerPos + kCameraHeight;
}

void PartBrokenBCameraStart::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting PartBrokenBCameraStart::GetBlendSetting() const
{
	return BlendSetting{
	.mode = BlendSetting::Mode::Lerp,
	.duration = kBlendDuration,
	.easingMode = EasingMode::EaseOut,
	.easingPower = kBlendEasingPower
	};
}

BlendSetting PartBrokenBCameraStart::GetOverrideBlendSetting() const
{
	//プレイヤーを中心にSlerpで回り込む
	Vector3 pivot = Vector3();
	if (auto cameraManager = m_owner.lock())
	{
		if (auto player = cameraManager->GetContext()->m_player.lock())
		{
			pivot = player->GetRigidBody().GetPos();
		}
	}
	return BlendSetting{
		.mode = BlendSetting::Mode::Slerp,
		.duration = kNextBlendDuration,
		.easingMode = EasingMode::EaseOutBack,
		.easingPower = 1.0f,
		.pivot = pivot
	};
}
