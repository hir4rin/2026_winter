#include "FinishingFirstCamera.h"
#include "FinishingSecondCamera.h"
#include "../CameraManager.h"
#include "PlayerFollowCamera.h"
#include <cmath>

namespace
{
	constexpr float kChangeTime = 150.0f;

	const Vector3 kGoalPos = Vector3(317.0f, 230.049f, 7290.884f);
	const Vector3 kGoalTarget = Vector3(319.362f, 196.337f, 6922.121f);

	constexpr float kCameraNear = 0.0f;//ニアクリップ
	constexpr float kCameraFar = 2500.0f;//ファークリップ
}

FinishingFirstCamera::FinishingFirstCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

FinishingFirstCamera::~FinishingFirstCamera()
{
}

void FinishingFirstCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_timer =0.0f;
	ResetBlend(data.pos, data.target);

	//TODO: ここで見せたい座標・注視点(m_goalPos/m_goalTarget)をセットする
	m_goalPos = kGoalPos;
	m_goalTarget = kGoalTarget;

	//ニアファーの調整
	SetCameraNearFar(kCameraNear, kCameraFar);
}

void FinishingFirstCamera::Update()
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

	//次のStateへ
	auto cameraManager = m_owner.lock();
	if (cameraManager && m_timer > kChangeTime)
	{
		cameraManager->ChangeState(std::make_shared<FinishingSecondCamera>(m_owner));
		return;
	}
}

void FinishingFirstCamera::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);


}

void FinishingFirstCamera::FixCameraPos()
{
	//TODO: 必要ならここでm_goalPos/m_goalTargetを計算する
}

void FinishingFirstCamera::CameraSetting()
{
}
