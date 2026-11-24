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
	//向きは前のカメラからではなく、着地点の正面(背中側から見る)
	m_angleH = atan2f(m_frame.front.x, m_frame.front.z);
	m_angleV = kAngleV;
	m_pos = data.pos;
	m_target = data.target;
	m_goalPos = data.pos;
	m_goalTarget = data.target;
	ResetBlend(data.pos, data.target);

	m_frameCount = 0.0f;
	m_goalFov = kDefaultFov;
}

void PlayerStartMovieTurnCamera::Update()
{
	m_frameCount += 1.0f;
	FixCameraPos();
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
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(-toTarget.y, horizontalDist);//プラスでカメラが上(FixCameraPosと同じ向き)
}

void PlayerStartMovieTurnCamera::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	auto player = cameraManager ? cameraManager->GetContext()->m_player.lock() : nullptr;
	const Vector3 focus = player ? player->GetCameraFocusPos() : m_frame.origin;

	//PlayerFollowCamera::FixCameraPosと同じ置き方
	m_goalTarget = focus + kCameraHeight;
	const Vector3 toCamera = Vector3(-sinf(m_angleH) * cosf(m_angleV), sinf(m_angleV), -cosf(m_angleH) * cosf(m_angleV));
	m_goalPos = m_goalTarget + toCamera * kToPlayerLength;
}

void PlayerStartMovieTurnCamera::CameraSetting()
{
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
