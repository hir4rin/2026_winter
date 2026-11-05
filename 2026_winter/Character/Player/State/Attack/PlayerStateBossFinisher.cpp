#include "PlayerStateBossFinisher.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"
#include <algorithm>
#include <cmath>

PlayerStateBossFinisher::PlayerStateBossFinisher(std::weak_ptr<Player> player) :
	PlayerState(player)
{
	auto owner = m_owner.lock();
	if (!owner)return;
	m_target = owner->m_bossFinisherTarget;
	owner->m_bossFinisherTarget.reset();
}

PlayerStateBossFinisher::~PlayerStateBossFinisher()
{
}

void PlayerStateBossFinisher::Enter()
{
	auto player = m_owner.lock();
	if (!player)return;
	player->SetIsGhost(true);
	player->m_rb.m_vel = Vector3(0, 0, 0);
	PlayAnim(player->GetAnimName("HitLargeB"), false, 0.5f, -1.0f, 6.0f);
}

void PlayerStateBossFinisher::Update()
{
	auto player = m_owner.lock();
	if (!player)return;

	//ボスが消えたら戻る
	if (m_target.expired())
	{
		Finish();
		return;
	}

	player->m_rb.m_vel = Vector3(0, 0, 0);
	if (m_isMoving)
	{
		m_moveFrame += 1.0f;
		const float t = std::clamp(m_moveFrame / m_moveTime, 0.0f, 1.0f);
		player->m_rb.m_pos = Vector3::EaseLerp(m_moveStart, m_moveGoal, t, m_moveEasing, m_movePower);
		if (t >= 1.0f)m_isMoving = false;
	}
	player->m_anim.Update();
}

void PlayerStateBossFinisher::Exit()
{
	auto player = m_owner.lock();
	if (!player)return;
	player->SetIsGhost(false);
	player->m_anim.SetRootMotionEnable(RootMotionCancel::None);
}

void PlayerStateBossFinisher::DebugDraw()
{
}

void PlayerStateBossFinisher::Warp(const Vector3& pos, const Vector3& front)
{
	auto player = m_owner.lock();
	if (!player)return;
	m_isMoving = false;
	player->m_rb.m_pos = pos;
	player->m_targetVec = front;
	player->SetRotY(atan2f(front.x, front.z) - DX_PI_F);
}

void PlayerStateBossFinisher::MoveTo(const Vector3& goal, float frame, EasingMode easing, float power)
{
	auto player = m_owner.lock();
	if (!player)return;
	m_moveStart = player->m_rb.m_pos;
	m_moveGoal = goal;
	m_moveFrame = 0.0f;
	m_moveTime = frame;
	m_moveEasing = easing;
	m_movePower = power;
	m_isMoving = true;
}

void PlayerStateBossFinisher::PlayAnim(const std::string& animName, bool isLoop, float speed, float endFrame, float blendFrame,
	RootMotionCancel cancel)
{
	auto player = m_owner.lock();
	if (!player)return;
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, animName, isLoop, speed, endFrame, 0.0f, blendFrame);
	player->m_anim.SetRootMotionEnable(cancel);
}

void PlayerStateBossFinisher::Finish()
{
	auto player = m_owner.lock();
	if (!player)return;
	player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
}

float PlayerStateBossFinisher::GetAnimFrame()const
{
	auto player = m_owner.lock();
	if (!player)return 0.0f;
	return player->m_anim.GetNowAnimFrame();
}

const ComboNode& PlayerStateBossFinisher::GetComboNode(int comboIndex)const
{
	return m_owner.lock()->GetComboNode(comboIndex);
}

const std::string& PlayerStateBossFinisher::GetAnimNameOf(const std::string& key)const
{
	return m_owner.lock()->GetAnimName(key);
}
