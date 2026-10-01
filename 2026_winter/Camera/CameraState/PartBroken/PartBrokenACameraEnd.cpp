#include "PartBrokenACameraEnd.h"
#include "../../CameraManager.h"
#include "Player.h"
#include "../PlayerFollowCamera.h"
#include "../Math/Matrix4x4.h"
#include "../System.h"
#include <cmath>


namespace
{
	constexpr float kBlendDuration = 40.0f;//ブレンドにかけるフレーム数
	constexpr float kBlendEasingPower = 2.0f;//ブレンドのイージング指数

	//PlayerFollowCameraの初期状態と合わせる
	constexpr float kToPlayerLength = 500.0f;//プレイヤーからカメラまでの距離(PlayerFollowCameraのkToPlayerLength * kToPlayerLengthScale)
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//注視点の高さ(Vector3)
	constexpr float kAngleV = DX_PI_F / 10;//見やすい垂直アングル(PlayerFollowCameraのkAngleH)
}

PartBrokenACameraEnd::PartBrokenACameraEnd(std::weak_ptr<CameraManager> owner):CameraStateBase(owner)
{
}

PartBrokenACameraEnd::~PartBrokenACameraEnd()
{
}

void PartBrokenACameraEnd::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_pos = data.pos;
	m_target = data.target;
	m_goalPos = data.pos;
	m_goalTarget = data.target;
	ResetBlend(data.pos, data.target);
	//前のStateが上書きブレンドを指定していたらそれを使う
	if (data.overrideSetting.mode != BlendSetting::Mode::None)
	{
		m_activeBlend = data.overrideSetting;
	}
}

void PartBrokenACameraEnd::Update()
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

		//PlayerFollowCameraの初期位置に着いたので通常カメラに戻す
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}
}

void PartBrokenACameraEnd::Exit()
{
	//PlayerFollowCameraに初期状態と同じ向きを渡す
	m_angleH = m_goalAngleH;
	m_angleV = m_goalAngleV;
}

void PartBrokenACameraEnd::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//プレイヤーの向いている方向の後ろにカメラを置く
	Vector3 forward = player->GetTargetVec();
	forward.y = 0.0f;
	if (forward.Magnitude() > 0.0f)
	{
		m_goalAngleH = atan2f(forward.x, forward.z);
	}
	m_goalAngleV = kAngleV;

	//注視点
	m_goalTarget = player->GetRigidBody().GetPos() + kCameraHeight;

	//PlayerFollowCamera::FixCameraPosと同じ計算
	auto rotY = Matrix4x4::MakeRotationY(m_goalAngleH);
	auto rotX = Matrix4x4::MakeRotationX(m_goalAngleV);

	auto CtoP = Vector3(0.0f, 0.0f, -kToPlayerLength);//プレイヤーからカメラへのベクトル
	//DxLibに変換
	auto CtoPVec = CtoP.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する
	auto rotXMat = Matrix4x4::ToDxLibMatrix(rotX);//回転行列を転置する
	auto RotCtoP = VTransform(CtoPVec, rotXMat);//回転させる
	RotCtoP = VTransform(RotCtoP, rotYMat);//回転させる

	auto pos = VAdd(RotCtoP, m_goalTarget.ToDxLibVector());//注視点に足す
	m_goalPos = Vector3::FromDxLibVector(pos);
}

void PartBrokenACameraEnd::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting PartBrokenACameraEnd::GetBlendSetting() const
{
	return BlendSetting{
	.mode = BlendSetting::Mode::Lerp,
	.duration = kBlendDuration,
	.easingMode = EasingMode::EaseInOut,
	.easingPower = kBlendEasingPower
	};
}

BlendSetting PartBrokenACameraEnd::GetOverrideBlendSetting() const
{
	return BlendSetting{
			.mode = BlendSetting::Mode::None,
			.duration = 10.0f,
			.easingMode = EasingMode::EaseIn,
			.easingPower = 0.5f,
			.pivot = Vector3()
	};
}
