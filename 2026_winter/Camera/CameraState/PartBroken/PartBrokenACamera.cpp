#include "PartBrokenACamera.h"
#include "../../CameraManager.h"
#include "Player.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../Math/Matrix4x4.h"
#include "../System.h"
#include <cmath>


namespace
{
	constexpr float kBlendDuration = 60.0f;//ブレンドにかけるフレーム数
	constexpr float kBlendEasingPower = 0.5f;//ブレンドのイージング指数

	constexpr float kStayFrame = 20.0f;//距離を変え始めるまでのフレーム数

	constexpr float kStartDistance = 350.0f;//プレイヤーからカメラまでの水平距離(開始)
	constexpr float kEndDistance = 200.0f;//プレイヤーからカメラまでの水平距離(終了)
	constexpr float kDistanceChangeFrame = 40.0f;//距離を変えるフレーム数
	constexpr float kDistanceEasingPower = 2.0f;//距離変化のイージング指数
	const Vector3 kCameraHeight = Vector3(0.0f, 300.0f, 0.0f);//カメラの高さ(Vector3)
	constexpr float kNextBlendDuration = 40.0f;//AssasinCameraStateへのSlerpのフレーム数

	constexpr float kCameraViewAngle = DX_PI_F / 3.0f;//カメラの視野角
	constexpr float kCameraNear = 10.0f;//ニアクリップ(0だとDxLibの深度計算が壊れて何も映らなくなる)
	constexpr float kCameraFar = 5000.0f;//ファークリップ(SceneMainと同じ値)
}

PartBrokenACamera::PartBrokenACamera(std::weak_ptr<CameraManager> owner):CameraStateBase(owner)
{
}

PartBrokenACamera::~PartBrokenACamera()
{
}

void PartBrokenACamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_pos = data.pos;
	m_target = data.target;
	m_goalPos = data.pos;
	m_goalTarget = data.target;
	ResetBlend(data.pos, data.target);
}

void PartBrokenACamera::Update()
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

void PartBrokenACamera::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void PartBrokenACamera::FixCameraPos()
{
	//TODO:m_goalPos / m_goalTarget を算出する

	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;
	auto enemy = player->GetPartBrokenTarget();
	if (!enemy)return;

	//PtoEVecを15度ずらす
	//向きは開始時に保存した方向で固定
	Vector3 PtoEVec = enemy->GetRigidBody().GetPos() - player->GetRigidBody().GetPos();
	PtoEVec.y = 0.0f;
	PtoEVec = PtoEVec.Normalize() * kStartDistance;

	float angle = DX_PI_F / 12;

	//水平方向の回転//敵との距離によってこの角度を帰る
	auto rotY = Matrix4x4::MakeRotationY(angle);

	//DxLibに変換
	auto PtoEVecDx = PtoEVec.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する

	auto RotPtoC = VTransform(PtoEVecDx, rotYMat);//回転させる
	RotPtoC = VAdd(RotPtoC, kCameraHeight.ToDxLibVector());


	auto pos = VAdd(RotPtoC, player->GetRigidBody().GetPos().ToDxLibVector());//プレイヤーの座標に足す
	m_goalPos = Vector3::FromDxLibVector(pos);

	//注視点
	//プレイヤーの頭のほうを見る
	m_goalTarget = player->GetWaistPos() + Vector3(0, 50.0f, 0);

}

void PartBrokenACamera::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting PartBrokenACamera::GetBlendSetting() const
{
	return BlendSetting{
	.mode = BlendSetting::Mode::Lerp,
	.duration = kBlendDuration,
	.easingMode = EasingMode::EaseOutBack,
	.easingPower = kBlendEasingPower
	};
}

BlendSetting PartBrokenACamera::GetOverrideBlendSetting() const
{
	//プレイヤーを中心にSlerpで回り込む
	Vector3 pivot = Vector3();
	if (auto cameraManager = m_owner.lock())
	{
		if (auto player = cameraManager->GetContext()->m_player.lock())
		{
			pivot = player->GetWaistPos();
		}
	}
	return BlendSetting{
		.mode = BlendSetting::Mode::Slerp,
		.duration = 30.0f,
		.easingMode = EasingMode::EaseIn,
		.easingPower = kBlendEasingPower,
		.pivot = pivot,
		.orbitDirection = OrbitDirection::CounterClockwise
	};
}
