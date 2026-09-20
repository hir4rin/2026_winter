#include "FinishingSecondCamera.h"
#include "../CameraManager.h"
#include "PlayerFollowCamera.h"
#include <cmath>

namespace
{
	constexpr float kChangeTime = 120.0f;

	const Vector3 kGoalPos = Vector3(144.775f, 61.433f, 6817.189f);
	const Vector3 kGoalTarget = Vector3(473.2f, 190.748f, 7240.6f);

}

FinishingSecondCamera::FinishingSecondCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

FinishingSecondCamera::~FinishingSecondCamera()
{
}

void FinishingSecondCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_timer =0.0f;
	ResetBlend(data.pos, data.target);

	//TODO: ここで見せたい座標・注視点(m_goalPos/m_goalTarget)をセットする
	m_goalPos = kGoalPos;
	m_goalTarget = kGoalTarget;
}

void FinishingSecondCamera::Update()
{
	m_timer += 1.0f;


	//途中でisUltを読んでもいい

	//Blend中はBlendのほうのlerp
	if (IsBlending())
	{
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	else
	{
		m_pos = m_goalPos;
		m_target = m_goalTarget;
	}

	//TODO: 一定フレーム経過したら元のStateに戻す
	auto cameraManager = m_owner.lock();
	if (cameraManager && m_timer > kChangeTime)
	{
		//cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}
}

void FinishingSecondCamera::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);


}

void FinishingSecondCamera::FixCameraPos()
{
	//TODO: 必要ならここでm_goalPos/m_goalTargetを計算する
}

void FinishingSecondCamera::CameraSetting()
{
}
