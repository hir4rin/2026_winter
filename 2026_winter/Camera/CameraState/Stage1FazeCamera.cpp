#include "Stage1FazeCamera.h"
#include "../CameraManager.h"
#include "PlayerFollowCamera.h"
#include <cmath>

namespace
{
	constexpr float kChangeTime = 120.0f;

	const Vector3 kGoalPos = Vector3(1021.3f, 652.9f, 521.4f);
	const Vector3 kGoalTarget = Vector3(857.811f, 514.783f, 798.375f);

}

Stage1FazeCamera::Stage1FazeCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

Stage1FazeCamera::~Stage1FazeCamera()
{
}

void Stage1FazeCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_timer =0.0f;
	ResetBlend(data.pos, data.target);

	//TODO: ここで見せたい座標・注視点(m_goalPos/m_goalTarget)をセットする
	m_goalPos = kGoalPos;
	m_goalTarget = kGoalTarget;
}

void Stage1FazeCamera::Update()
{
	m_timer += 1.0f;

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
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}
}

void Stage1FazeCamera::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);

	
}

void Stage1FazeCamera::FixCameraPos()
{
	//TODO: 必要ならここでm_goalPos/m_goalTargetを計算する
}

void Stage1FazeCamera::CameraSetting()
{
}
