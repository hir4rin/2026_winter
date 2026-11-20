#include "PlayerStateStartMovie.h"
#include "Character/Player/Player.h"
#include "Camera/CameraManager.h"
#include "Battle/BattleManager.h"
#include "Main/System.h"
#include <algorithm>
#include <cmath>

namespace
{
	using F = PlayerStartMovieFrame;
	constexpr float kLiftOffFrame = F::kJumpStartFrame + 8.0f;//JumpUpで体が浮くフレーム
	constexpr float kApexFrame = F::kJumpStartFrame + F::kCut3Frame;//着地点の真上に着く
	constexpr float kAttackStartFrame = kApexFrame - 10.0f;//空中強攻撃の振りかぶり

	constexpr float kApexHeight = 1966.0f;//着地点の上の高さ
	constexpr float kLeapEasingPower = 2.0f;
	constexpr float kPlungeEasingPower = 2.5f;

	constexpr float kBlendFrame = 5.0f;
	constexpr float kLandAnimSpeed = 0.3f;//着地のポーズを長く見せる
	constexpr float kLandAnimEndFrame = 19.0f;//PlayerStateAttackLandingと同じ
	constexpr float kStandBlendFrame = 30.0f;

	constexpr float kShakePower = 12.0f;
	constexpr float kShakeTime = 24.0f;
	constexpr float kFovPunchAngle = 8.0f;
	constexpr float kFovPunchTime = 24.0f;
	constexpr float kFovPunchInFrame = 2.0f;
	constexpr float kFovPunchOutPower = 2.0f;
}

PlayerStateStartMovie::PlayerStateStartMovie(std::weak_ptr<Player> player, const PlayerStartMovieFrame& frame) :
	PlayerState(player),
	m_frame(frame)
{
}

PlayerStateStartMovie::~PlayerStateStartMovie()
{
}

void PlayerStateStartMovie::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	System::GetInstance().GetBattleMgr()->SetIsEventPlaying(true);

	//屋上は当たり判定が無く、通りの見えない壁も抜けるので着地まで切る
	player->SetIsActive(false);
	player->m_rb.m_vel = Vector3(0, 0, 0);
	player->m_targetVec = m_frame.front;
	player->SetRotY(atan2f(m_frame.front.x, m_frame.front.z) - DX_PI_F);
	SetPos(*player, m_frame.RoofPos());

	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Idle"), true, 1.0f, -1.0f, 0.0f, 0.0f);
	m_phase = Phase::Roof;
	m_frameCount = 0.0f;
	m_isAttackAnim = false;
}

void PlayerStateStartMovie::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	//実フレームで進める(カメラと合わせる)
	m_frameCount += 1.0f;
	player->m_rb.m_vel = Vector3(0, 0, 0);

	switch (m_phase)
	{
	case Phase::Roof:
		if (m_frameCount >= F::kJumpStartFrame)
		{
			player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JumpUp"), false, 1.0f, -1.0f, 0.0f, kBlendFrame);
			m_phase = Phase::Leap;
		}
		break;
	case Phase::Leap:
		UpdateLeap(*player);
		break;
	case Phase::Plunge:
		UpdatePlunge(*player);
		break;
	case Phase::Land:
		if (player->m_anim.GetAnimEndFlag())
		{
			player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Idle"), true, 1.0f, -1.0f, 0.0f, kStandBlendFrame);
			m_phase = Phase::Stand;
		}
		break;
	case Phase::Stand:
		break;
	}

	if (m_frameCount >= F::kTotalFrame)
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
		return;
	}

	player->m_anim.Update();
}

void PlayerStateStartMovie::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->SetIsActive(true);
	System::GetInstance().GetBattleMgr()->SetIsEventPlaying(false);
}

void PlayerStateStartMovie::DebugDraw()
{
}

void PlayerStateStartMovie::UpdateLeap(Player& player)
{
	//振りかぶり//空中強攻撃と同じアニメ・速さ・止めるフレーム
	if (!m_isAttackAnim && m_frameCount >= kAttackStartFrame)
	{
		const ComboNode& node = player.GetComboNode(ComboIndex::AirHeavyAttack1);
		player.m_anim.ChangeAnimWithModelHandle(player.m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame, 0.0f, kBlendFrame);
		m_isAttackAnim = true;
	}

	if (m_frameCount < kLiftOffFrame)return;

	//着地点の真上へ、上がりながら速度を落とす
	const float s = std::clamp((m_frameCount - kLiftOffFrame) / (kApexFrame - kLiftOffFrame), 0.0f, 1.0f);
	const float eased = Easing::Apply(EasingMode::EaseOut, s, kLeapEasingPower);
	const Vector3 roof = m_frame.RoofPos();
	const Vector3 apex = GetApexPos();
	Vector3 pos = Vector3::Lerp(roof, apex, eased);
	SetPos(player, pos);

	if (s >= 1.0f)m_phase = Phase::Plunge;
}

void PlayerStateStartMovie::UpdatePlunge(Player& player)
{
	//真下へ加速しながら落ちる
	const float s = std::clamp((m_frameCount - kApexFrame) / (F::kLandFrame - kApexFrame), 0.0f, 1.0f);
	const float eased = Easing::Apply(EasingMode::EaseIn, s, kPlungeEasingPower);
	SetPos(player, Vector3::Lerp(GetApexPos(), m_frame.origin, eased));

	if (s >= 1.0f)Land(player);
}

void PlayerStateStartMovie::Land(Player& player)
{
	SetPos(player, m_frame.origin);
	player.SetIsActive(true);
	player.SetIsFloor(true);
	player.m_isGround = true;

	player.m_anim.ChangeAnimWithModelHandle(player.m_modelHandle, player.GetAnimName("HeavyAttackLanding"), false, kLandAnimSpeed, kLandAnimEndFrame, 0.0f, kBlendFrame);

	if (auto camera = player.GetCameraManager().lock())
	{
		camera->StartCameraShake(kShakePower, kShakeTime);
		camera->StartFovPunch(kFovPunchAngle, kFovPunchTime, kFovPunchInFrame, kFovPunchOutPower);
	}
	m_phase = Phase::Land;
}

void PlayerStateStartMovie::SetPos(Player& player, const Vector3& pos)
{
	//当たり判定を切っている間はApplyPosが呼ばれないので、モデルもここで動かす
	player.m_rb.m_pos = pos;
	player.UpdateAngleAndPos();
}

Vector3 PlayerStateStartMovie::GetApexPos()const
{
	return m_frame.Point(0.0f, kApexHeight, 0.0f);
}
