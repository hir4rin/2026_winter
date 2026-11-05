#include "BossFinisherCamera.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kDegToRad = DX_PI_F / 180.0f;
}

BossFinisherCamera::BossFinisherCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

BossFinisherCamera::~BossFinisherCamera()
{
}

void BossFinisherCamera::Enter(CameraData data)
{
	m_pos = data.pos;
	m_target = data.target;
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_goalFov = data.fov;
}

void BossFinisherCamera::Update()
{
	if (!m_hasShot)return;
	m_shotFrame += 1.0f;//実フレームで進める
	ApplyShot();
}

void BossFinisherCamera::Exit()
{
}

void BossFinisherCamera::SetShot(const Shot& shot)
{
	m_shot = shot;
	m_shotFrame = 0.0f;
	m_hasShot = true;
	ApplyShot();
}

void BossFinisherCamera::ApplyShot()
{
	const float t = std::clamp(m_shotFrame / m_shot.frame, 0.0f, 1.0f);
	m_pos = Vector3::Lerp(m_shot.startPos, m_shot.endPos, t);
	m_target = Vector3::Lerp(m_shot.startTarget, m_shot.endTarget, t);
	m_goalFov = std::lerp(m_shot.startFov, m_shot.endFov, t) * kDegToRad;
	m_fov = m_goalFov;
}
