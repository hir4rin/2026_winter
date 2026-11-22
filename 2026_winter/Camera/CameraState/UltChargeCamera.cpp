#include "UltChargeCamera.h"
#include "../CameraManager.h"
#include "PlayerFollowCamera.h"
#include "Character/Player/Player.h"
#include <cmath>

namespace
{
	constexpr float kDegToRad = DX_PI_F / 180.0f;

	//刀が伸びる前(正面のローアングル)
	constexpr float kNearDistance = 230.0f;//プレイヤーからの水平距離
	constexpr float kNearHeight = 40.0f;//カメラの高さ
	constexpr float kNearTargetHeight = 115.0f;//注視点の高さ
	constexpr float kNearFov = 45.0f * kDegToRad;
	constexpr float kNearSideAngle = 15.0f * kDegToRad;//正面からプレイヤーの右(刀のある側)へ回す角度
	//刀が伸びきった後//刀は右後ろの斜め下へ伸びるので、右へ回って横から見る
	constexpr float kFarDistance = 300.0f;
	constexpr float kFarHeight = 75.0f;
	constexpr float kFarTargetHeight = 95.0f;
	constexpr float kFarFov = 55.0f * kDegToRad;
	constexpr float kFarSideAngle = 40.0f * kDegToRad;
	constexpr float kFarTargetSide = 50.0f;//注視点を刀の側へずらす

	//手持ちの揺れ
	constexpr float kSwayPower = 1.5f;
	constexpr float kSwaySpeed = 0.045f;

	//前へ回り込むブレンド
	constexpr float kBlendDuration = 12.0f;
	constexpr float kBlendEasingPower = 2.0f;

	//いつものカメラへ戻る
	constexpr float kReturnFrame = 18.0f;
	constexpr float kReturnEasingPower = 3.0f;

	//PlayerFollowCameraの初期状態と合わせる
	constexpr float kFollowLength = 350.0f;
	constexpr float kFollowTargetHeight = 150.0f;
	constexpr float kFollowAngleV = DX_PI_F / 10.0f;
}

UltChargeCamera::UltChargeCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

UltChargeCamera::~UltChargeCamera()
{
}

void UltChargeCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;

	//プレイヤーの前を決める
	m_front = Vector3(sinf(data.angleH), 0.0f, cosf(data.angleH));
	auto cameraManager = m_owner.lock();
	if (cameraManager)
	{
		if (auto player = cameraManager->GetContext()->m_player.lock())
		{
			Vector3 vec = player->GetTargetVec();
			vec.y = 0.0f;
			if (vec.Magnitude() > 0.0f) m_front = vec.Normalize();
		}
	}

	ResetBlend(data.pos, data.target);
	m_goalFov = kNearFov;
}

void UltChargeCamera::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;
	m_frame += 1.0f;

	//いつものカメラへ回り込んで戻る
	if (m_isReturn)
	{
		m_returnFrame += 1.0f;
		float t = std::clamp(m_returnFrame / kReturnFrame, 0.0f, 1.0f);
		t = Easing::Apply(EasingMode::EaseOut, t, kReturnEasingPower);

		Vector3 goalPos, goalTarget;
		CalcFollowPos(goalPos, goalTarget);
		//プレイヤーの足元を中心に、水平の角度・距離・高さで補間する(体を突き抜けないように)
		const Vector3 center = player->GetRigidBody().GetPos();
		const Vector3 from = m_returnStartPos - center;
		const Vector3 to = goalPos - center;
		float angleFrom = atan2f(from.x, from.z);
		float angleTo = atan2f(to.x, to.z);
		float diff = angleTo - angleFrom;
		while (diff > DX_PI_F) diff -= DX_PI_F * 2.0f;
		while (diff < -DX_PI_F) diff += DX_PI_F * 2.0f;
		const float angle = angleFrom + diff * t;
		const float dist = std::lerp(Vector3(from.x, 0.0f, from.z).Magnitude(), Vector3(to.x, 0.0f, to.z).Magnitude(), t);
		const float height = std::lerp(from.y, to.y, t);
		m_pos = center + Vector3(sinf(angle) * dist, height, cosf(angle) * dist);
		m_target = Vector3::Lerp(m_returnStartTarget, goalTarget, t);
		m_fov = std::lerp(m_returnStartFov, kDefaultFov, t);
		m_goalFov = m_fov;

		if (m_returnFrame >= kReturnFrame)
		{
			cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		}
		return;
	}

	float fov = kNearFov;
	CalcChargePos(m_goalPos, m_goalTarget, fov);
	m_goalFov = fov;

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

void UltChargeCamera::Exit()
{
	//PlayerFollowCameraにプレイヤーの後ろの向きを渡す
	m_angleH = atan2f(m_front.x, m_front.z);
	m_angleV = kFollowAngleV;
}

void UltChargeCamera::StartReturn()
{
	if (m_isReturn)return;
	m_isReturn = true;
	m_returnFrame = 0.0f;
	m_returnStartPos = m_pos;
	m_returnStartTarget = m_target;
	m_returnStartFov = GetFov();
}

void UltChargeCamera::CalcChargePos(Vector3& pos, Vector3& target, float& fov) const
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//刀の伸び具合で引く
	const float rate = player->GetUltBladeRate();
	const float dist = std::lerp(kNearDistance, kFarDistance, rate);
	const float height = std::lerp(kNearHeight, kFarHeight, rate);
	const float targetHeight = std::lerp(kNearTargetHeight, kFarTargetHeight, rate);
	fov = std::lerp(kNearFov, kFarFov, rate);

	//前の向きをプレイヤーの右へ回す
	const float side = std::lerp(kNearSideAngle, kFarSideAngle, rate);
	const Vector3 dir = Vector3(m_front.x * cosf(side) + m_front.z * sinf(side), 0.0f, -m_front.x * sinf(side) + m_front.z * cosf(side));
	const Vector3 right = Vector3(m_front.z, 0.0f, -m_front.x);

	const Vector3 center = player->GetRigidBody().GetPos();
	pos = center + dir * dist + Vector3(0.0f, height, 0.0f);
	target = center + Vector3(0.0f, targetHeight, 0.0f) + right * (kFarTargetSide * rate);

	//手持ちの揺れ
	pos += Vector3(sinf(m_frame * kSwaySpeed * 1.3f), cosf(m_frame * kSwaySpeed), 0.0f) * kSwayPower;
}

void UltChargeCamera::CalcFollowPos(Vector3& pos, Vector3& target) const
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//PlayerFollowCamera::FixCameraPosと同じ位置(プレイヤーの後ろ)
	const float angleH = atan2f(m_front.x, m_front.z);
	target = player->GetCameraFocusPos() + Vector3(0.0f, kFollowTargetHeight, 0.0f);
	const Vector3 toCamera = Vector3(-sinf(angleH) * cosf(kFollowAngleV), sinf(kFollowAngleV), -cosf(angleH) * cosf(kFollowAngleV));
	pos = target + toCamera * kFollowLength;
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting UltChargeCamera::GetBlendSetting() const
{
	//プレイヤーを中心に前へ回り込む
	auto cameraManager = m_owner.lock();
	std::shared_ptr<Player> player;
	if (cameraManager) player = cameraManager->GetContext()->m_player.lock();

	return BlendSetting{
		.mode = BlendSetting::Mode::Slerp,
		.duration = kBlendDuration,
		.easingMode = EasingMode::EaseOut,
		.easingPower = kBlendEasingPower,
		.pivot = (player) ? player->GetRigidBody().GetPos() : Vector3(),
	};
}
