#include "PlayerStartMovieLandCamera.h"
#include "PlayerStartMovieTurnCamera.h"
#include "Camera/CameraManager.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kDegToRad = DX_PI_F / 180.0f;

	//着地点の正面からの角度(+で右回り)・距離・高さ
	constexpr float kStartAngle = -35.0f;
	constexpr float kEndAngle = -12.0f;
	constexpr float kStartDist = 280.0f;
	constexpr float kEndDist = 250.0f;
	constexpr float kHeight = 50.0f;
	constexpr float kTargetHeight = 75.0f;
	constexpr float kFov = 45.0f;
	constexpr float kEasingPower = 2.0f;
}

PlayerStartMovieLandCamera::PlayerStartMovieLandCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame) :
	CameraStateBase(owner),
	m_frame(frame)
{
}

PlayerStartMovieLandCamera::~PlayerStartMovieLandCamera()
{
}

void PlayerStartMovieLandCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_frameCount = 0.0f;
	m_goalFov = kFov * kDegToRad;
	CalcGoal();
	ResetBlend(data.pos, data.target);
}

void PlayerStartMovieLandCamera::Update()
{
	m_frameCount += 1.0f;
	if (m_frameCount >= PlayerStartMovieFrame::kLandPoseFrame)
	{
		if (auto cameraManager = m_owner.lock())
		{
			cameraManager->ChangeState(std::make_shared<PlayerStartMovieTurnCamera>(m_owner, m_frame));
		}
		return;
	}

	CalcGoal();
	if (IsBlending())
	{
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	else
	{
		m_pos = m_goalPos;
		m_target = m_goalTarget;
	}
}

void PlayerStartMovieLandCamera::Exit()
{
}

void PlayerStartMovieLandCamera::CalcGoal()
{
	float t = std::clamp(m_frameCount / PlayerStartMovieFrame::kLandPoseFrame, 0.0f, 1.0f);
	t = Easing::Apply(EasingMode::EaseOut, t, kEasingPower);
	m_goalPos = m_frame.Orbit(std::lerp(kStartAngle, kEndAngle, t), std::lerp(kStartDist, kEndDist, t), kHeight);
	m_goalTarget = m_frame.Point(0.0f, kTargetHeight, 0.0f);
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting PlayerStartMovieLandCamera::GetBlendSetting() const
{
	//C5からはカット
	return {};
}
