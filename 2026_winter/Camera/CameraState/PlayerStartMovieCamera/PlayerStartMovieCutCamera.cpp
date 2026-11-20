#include "PlayerStartMovieCutCamera.h"
#include "PlayerStartMovieLandCamera.h"
#include "Camera/CameraManager.h"
#include "Character/Player/Player.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kDegToRad = DX_PI_F / 180.0f;
	constexpr float kEasingPower = 2.0f;
}

PlayerStartMovieCutCamera::PlayerStartMovieCutCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame) :
	CameraStateBase(owner),
	m_frame(frame)
{
	BuildCuts();
}

PlayerStartMovieCutCamera::~PlayerStartMovieCutCamera()
{
}

void PlayerStartMovieCutCamera::BuildCuts()
{
	using F = PlayerStartMovieFrame;
	const Vector3 roof = Vector3(F::kRoofSide, F::kRoofHeight, F::kRoofFront);

	m_cuts = {
		//C1 通りから屋上の縁に立つ姿を見上げる
		{ Vector3(-260.0f, 220.0f, 950.0f), Vector3(-200.0f, 260.0f, 650.0f),
		  roof + Vector3(0.0f, -150.0f, 0.0f), roof + Vector3(0.0f, -80.0f, 0.0f),
		  50.0f, 45.0f, F::kCut1Frame, Follow::None, EasingMode::EaseOut },
		//C2 横顔のアップ
		{ roof + Vector3(120.0f, 165.0f, 95.0f), roof + Vector3(110.0f, 170.0f, 85.0f),
		  roof + Vector3(0.0f, 160.0f, 0.0f), roof + Vector3(0.0f, 160.0f, 10.0f),
		  35.0f, 33.0f, F::kCut2Frame, Follow::None, EasingMode::EaseInOut },
		//C3 真横に固定//着地点の上へ飛んで振りかぶる
		{ Vector3(-800.0f, 1900.0f, -300.0f), Vector3(-800.0f, 1900.0f, -300.0f),
		  Vector3(80.0f, 1900.0f, -300.0f), Vector3(80.0f, 1900.0f, -300.0f),
		  60.0f, 60.0f, F::kCut3Frame, Follow::None, EasingMode::EaseInOut },
		//C4 頭の上から真下へ付いていく
		{ Vector3(0.0f, 380.0f, -140.0f), Vector3(0.0f, 330.0f, -120.0f),
		  Vector3(0.0f, -300.0f, 90.0f), Vector3(0.0f, -300.0f, 90.0f),
		  60.0f, 72.0f, F::kCut4Frame, Follow::All, EasingMode::EaseIn },
		//C5 地面すれすれから見上げる//着地
		{ Vector3(-230.0f, 35.0f, 280.0f), Vector3(-225.0f, 35.0f, 270.0f),
		  Vector3(0.0f, 90.0f, 0.0f), Vector3(0.0f, 90.0f, 0.0f),
		  55.0f, 55.0f, F::kCut5Frame, Follow::Target, EasingMode::EaseInOut },
	};
}

void PlayerStartMovieCutCamera::Enter(CameraData data)
{
	m_angleH = atan2f(m_frame.front.x, m_frame.front.z);
	m_angleV = data.angleV;
	m_index = 0;
	m_cutFrame = 0.0f;
	ApplyCut();
	m_activeBlend = {};
}

void PlayerStartMovieCutCamera::Update()
{
	//実フレームで進める
	m_cutFrame += 1.0f;
	if (m_cutFrame >= m_cuts[m_index].frame)
	{
		m_cutFrame = 0.0f;
		++m_index;
		if (m_index >= static_cast<int>(m_cuts.size()))
		{
			m_index = static_cast<int>(m_cuts.size()) - 1;
			if (auto cameraManager = m_owner.lock())
			{
				cameraManager->ChangeState(std::make_shared<PlayerStartMovieLandCamera>(m_owner, m_frame));
			}
			return;
		}
	}
	ApplyCut();
}

void PlayerStartMovieCutCamera::Exit()
{
}

void PlayerStartMovieCutCamera::ApplyCut()
{
	const Cut& cut = m_cuts[m_index];
	const float t = std::clamp(m_cutFrame / cut.frame, 0.0f, 1.0f);
	const Vector3 pos = Vector3::EaseLerp(cut.startPos, cut.endPos, t, cut.easing, kEasingPower);
	const Vector3 target = Vector3::EaseLerp(cut.startTarget, cut.endTarget, t, cut.easing, kEasingPower);

	m_pos = ToWorld(pos, cut.follow == Follow::All);
	m_target = ToWorld(target, cut.follow != Follow::None);
	m_goalFov = std::lerp(cut.startFov, cut.endFov, t) * kDegToRad;
	m_fov = m_goalFov;
}

Vector3 PlayerStartMovieCutCamera::ToWorld(const Vector3& local, bool isPlayer)const
{
	if (!isPlayer)return m_frame.Point(local);

	auto cameraManager = m_owner.lock();
	auto player = cameraManager ? cameraManager->GetContext()->m_player.lock() : nullptr;
	if (!player)return m_frame.Point(local);
	return player->GetRigidBody().GetPos() + m_frame.Dir(local);
}
