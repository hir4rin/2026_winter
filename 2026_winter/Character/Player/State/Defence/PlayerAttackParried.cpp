#include "PlayerAttackParried.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"
#include "Main/Input.h"
#include "Main/System.h"

namespace
{
	//ひるんでいる時間(frame)//仮
	//敵の反撃の予備動作(EnemyAttackPreの40F)より短くして、ひるみが解けた後に回避が間に合うようにする
	constexpr float kParriedTime = 30.0f;

	constexpr float kAnimEndFrame = 27.0f;
	constexpr float kAnimMoveEndFrame = 20.0f;
}

PlayerAttackParried::PlayerAttackParried(std::weak_ptr<Player> player)
	:PlayerState(player)
{
}

PlayerAttackParried::~PlayerAttackParried()
{
}

void PlayerAttackParried::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//敵のパリィ後だから内部ターゲットとかで十分
	auto enemy = player->GetAttackTarget();
	if (!enemy)return;


	//よろめく方向
	m_moveVec = player->GetRigidBody().GetPos() - enemy->GetRigidBody().GetPos();
	m_moveVec.y = 0.0f;


	player->m_anim.ChangeAnim(player->GetAnimName("AttackBlock"), false,0.5f, kAnimEndFrame);
	//その場で止まる
	player->m_rb.m_vel = Vector3(0, 0, 0);
	m_timer = 0.0f;
}

void PlayerAttackParried::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//よろめく
	if (player->m_anim.IsAnimFrameBetween(0.0f, kAnimMoveEndFrame))
	{
		player->m_rb.m_vel += m_moveVec.Normalize() * 3.0f;
	}

	player->m_anim.Update(player->m_ownTimeScale);



	m_timer += System::GetInstance().GetTimeScale() * player->m_ownTimeScale;
	//if (m_timer < kParriedTime)return;

	if (player->m_anim.GetNowAnimFrame() >= kAnimEndFrame)
	{
		if (input.IsLeftStickInput())
		{
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
		return;
	}

	
}

void PlayerAttackParried::Exit()
{
}

void PlayerAttackParried::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:AttackParried %.0f/%.0f", m_timer, kParriedTime);
#endif
}
