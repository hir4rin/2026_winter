#include "EnemyHitDrop.h"
#include "../../EnemyBase.h"

EnemyHitDrop::EnemyHitDrop(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info) :
	EnemyStateBase(owner),
	m_info(info)
{
}

EnemyHitDrop::~EnemyHitDrop()
{
}

void EnemyHitDrop::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Hit"), false);
	//HitInfoの通りに速度を与えるだけ//縦方向の重力はEnemySwordman::Updateがかけている
	owner->m_rb.m_vel = m_info.knockBackVel;
}

void EnemyHitDrop::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);

	//着地したら、死亡予定ならDie、そうでなければKnockBack
	if (owner->IsFloor())
	{
		owner->m_rb.m_vel = Vector3(0, 0, 0);
		if (m_info.willDie)
		{
			owner->ChangeState(std::make_shared<EnemyDie>(owner));
		}
		else
		{
			owner->ChangeState(std::make_shared<EnemyKnockBack>(owner));
		}
	}
}

void EnemyHitDrop::Exit()
{
}

void EnemyHitDrop::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:HitDrop");
}
