#include "AirAssasinCameraStart.h"
#include "AirAssasinCamera.h"
#include "../../CameraManager.h"
#include "Character/Player/Player.h"
#include "../Character/Enemy/EnemyBase.h"
#include "Main/System.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kBlendDuration = 30.0f;//このStateに入るときのブレンドフレーム数
	constexpr float kBlendEasingPower = 2.0f;//このStateに入るときのイージング指数
	constexpr float kStayFrame = 60.0f;//このStateに留まるフレーム数
	constexpr float kCameraDistance = 300.0f;//プレイヤーからカメラまでの水平距離(PlayerFollowは350)
	constexpr float kSideAngleDeg = 5.0f;//敵→プレイヤーの延長線からずらす角度(度)
	constexpr float kDegToRad = DX_PI_F / 180.0f;
	const Vector3 kCameraHeight = Vector3(0.0f, 250.0f, 0.0f);//カメラの高さ
	const Vector3 kTargetHeight = Vector3(0.0f, 100.0f, 0.0f);//注視点の高さ
	constexpr float kTargetForward = 100.0f;//注視点をプレイヤーから敵側へずらす距離

	constexpr float kNextBlendDuration = 40.0f;//AirAssasinCameraStateへのSlerpのフレーム数

	constexpr float kCameraViewAngleEnd = 40.0f * kDegToRad;//狭めきったときの視野角
	constexpr EasingMode kFovEasingMode = EasingMode::EaseOut;//視野角を狭めるときのイージング
	constexpr float kFovEasingPower = 2.0f;//視野角を狭めるときのイージング指数
	constexpr float kFovDuration = 45.0f;//視野角を狭めるフレーム数
	constexpr EasingMode kNextFovEasingMode = EasingMode::EaseOut;//AirAssasinCameraStateで視野角を戻すときのイージング
	constexpr float kNextFovEasingPower = 2.0f;//AirAssasinCameraStateで視野角を戻すときのイージング指数
	constexpr float kNextFovDuration = 40.0f;//AirAssasinCameraStateで視野角を戻すフレーム数
	constexpr float kCameraNear = 10.0f;//ニアクリップ(0だとDxLibの深度計算が壊れて何も映らなくなる)
	constexpr float kCameraFar = 5000.0f;//ファークリップ(SceneMainと同じ値)
}

AirAssasinCameraStart::AirAssasinCameraStart(std::weak_ptr<CameraManager> owner) : CameraStateBase(owner)
{
}

AirAssasinCameraStart::~AirAssasinCameraStart()
{
}

void AirAssasinCameraStart::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_pos = data.pos;
	m_target = data.target;
	m_goalPos = data.pos;
	m_goalTarget = data.target;
	ResetBlend(data.pos, data.target);
	//入るときのブレンドで視野角を狭める
	m_goalFov = kCameraViewAngleEnd;

	//今のカメラに近いほうへずらす
	if (auto cameraManager = m_owner.lock())
	{
		if (auto player = cameraManager->GetContext()->m_player.lock())
		{
			Vector3 playerPos = player->GetRigidBody().GetPos();
			if (auto enemy = player->GetAssasinTarget())
			{
				m_forward = enemy->GetRigidBody().GetPos() - playerPos;
			}
			else
			{
				m_forward = player->GetForward();
			}
			m_forward.y = 0.0f;
			if (m_forward.Magnitude() < 0.0001f)
			{
				m_forward = Vector3(0.0f, 0.0f, 1.0f);
			}
			m_forward = m_forward.Normalize();

			Vector3 right = Vector3(m_forward.z, 0.0f, -m_forward.x);
			Vector3 toCamera = data.pos - playerPos;
			m_sideSign = (right.Dot(toCamera) >= 0.0f) ? 1.0f : -1.0f;
		}
	}

	SetCameraNearFar(kCameraNear, kCameraFar);
}

void AirAssasinCameraStart::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;

	FixCameraPos();

	m_timer += 1.0f * System::GetInstance().GetTimeScale();

	if (IsBlending())
	{
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	else
	{
		m_pos = m_goalPos;
		m_target = m_goalTarget;
	}

	//AirAssasinCameraへの切り替えはPlayerStateAirAssasinで行う(Assasinと同じ)
	//if (m_timer > kStayFrame)
	//{
	//	cameraManager->ChangeState(std::make_shared<AirAssasinCameraState>(m_owner));
	//	return;
	//}
}

void AirAssasinCameraStart::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void AirAssasinCameraStart::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();

	//敵の方向(暗殺対象がいなくなったら前の向きのまま)
	if (auto enemy = player->GetAssasinTarget())
	{
		Vector3 toEnemy = enemy->GetRigidBody().GetPos() - playerPos;
		toEnemy.y = 0.0f;
		if (toEnemy.Magnitude() > 0.0001f)
		{
			m_forward = toEnemy.Normalize();
		}
	}

	//プレイヤーの右方向(左手系・Y上:前(sin,cos)に対して右(cos,-sin))
	Vector3 right = Vector3(m_forward.z, 0.0f, -m_forward.x);
	Vector3 back = m_forward * -1.0f;

	//敵→プレイヤーの延長線から、今のカメラに近いほうへずらす
	float rad = kSideAngleDeg * kDegToRad;
	Vector3 dir = (back * cosf(rad) + right * (sinf(rad) * m_sideSign)).Normalize();

	m_goalPos = playerPos + dir * kCameraDistance + kCameraHeight;
	m_goalTarget = playerPos + m_forward * kTargetForward + kTargetHeight;
}

void AirAssasinCameraStart::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting AirAssasinCameraStart::GetBlendSetting() const
{
	return BlendSetting{
		.mode = BlendSetting::Mode::Lerp,
		.duration = kBlendDuration,
		.easingMode = EasingMode::EaseOut,
		.easingPower = kBlendEasingPower,
		.fovEasingMode = kFovEasingMode,
		.fovEasingPower = kFovEasingPower,
		.fovDuration = kFovDuration
	};
}

BlendSetting AirAssasinCameraStart::GetOverrideBlendSetting()const
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
		.easingMode = EasingMode::EaseIn,
		.easingPower = 1.0f,
		.pivot = pivot,
		.fovEasingMode = kNextFovEasingMode,
		.fovEasingPower = kNextFovEasingPower,
		.fovDuration = kNextFovDuration
	};
}

