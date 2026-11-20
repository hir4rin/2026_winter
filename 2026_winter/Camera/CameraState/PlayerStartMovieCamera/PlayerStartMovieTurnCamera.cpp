#include "PlayerStartMovieTurnCamera.h"
#include "Camera/CameraManager.h"
#include "Camera/CameraState/PlayerFollowCamera.h"
#include "Character/Player/Player.h"
#include <cmath>

namespace
{
	//PlayerFollowCameraの初期状態と合わせる
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);
	constexpr float kToPlayerLength = 350.0f;
	constexpr float kAngleV = DX_PI_F / 10.0f;

	constexpr float kBlendEasingPower = 2.0f;
}

PlayerStartMovieTurnCamera::PlayerStartMovieTurnCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame) :
	CameraStateBase(owner),
	m_frame(frame)
{
}

PlayerStartMovieTurnCamera::~PlayerStartMovieTurnCamera()
{
}

void PlayerStartMovieTurnCamera::Enter(CameraData data)
{
	m_angleH = atan2f(m_frame.front.x, m_frame.front.z);
	m_angleV = kAngleV;
	m_frameCount = 0.0f;
	m_goalFov = kDefaultFov;
	CalcGoal();
	ResetBlend(data.pos, data.target);
}

void PlayerStartMovieTurnCamera::Update()
{
	m_frameCount += 1.0f;
	CalcGoal();
	if (IsBlending())
	{
		UpdateBlend(m_goalPos, m_goalTarget);
		return;
	}

	m_pos = m_goalPos;
	m_target = m_goalTarget;
	if (m_frameCount >= PlayerStartMovieFrame::kTurnFrame)
	{
		if (auto cameraManager = m_owner.lock())
		{
			cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		}
	}
}

void PlayerStartMovieTurnCamera::Exit()
{
}

void PlayerStartMovieTurnCamera::CalcGoal()
{
	auto cameraManager = m_owner.lock();
	auto player = cameraManager ? cameraManager->GetContext()->m_player.lock() : nullptr;
	const Vector3 focus = player ? player->GetCameraFocusPos() : m_frame.origin;

	//PlayerFollowCamera::FixCameraPosと同じ置き方
	m_goalTarget = focus + kCameraHeight;
	const Vector3 toCamera = Vector3(-sinf(m_angleH) * cosf(m_angleV), sinf(m_angleV), -cosf(m_angleH) * cosf(m_angleV));
	m_goalPos = m_goalTarget + toCamera * kToPlayerLength;
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting PlayerStartMovieTurnCamera::GetBlendSetting() const
{
	//プレイヤーを中心に右から背中へ回り込む
	return BlendSetting{
		.mode = BlendSetting::Mode::Slerp,
		.duration = PlayerStartMovieFrame::kTurnFrame,
		.easingMode = EasingMode::EaseInOut,
		.easingPower = kBlendEasingPower,
		.pivot = m_frame.origin,
		.orbitDirection = OrbitDirection::Clockwise,
	};
}
