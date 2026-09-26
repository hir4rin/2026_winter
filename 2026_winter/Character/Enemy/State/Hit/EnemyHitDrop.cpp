#include "EnemyHitDrop.h"
#include "../../EnemyBase.h"
#include "../Game.h"

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

	//重力を足す
	owner->m_rb.m_vel.y += -Game::kGravity;

	owner->m_anim.Update(owner->m_ownTimeScale);

	//着地したら、死亡予定ならDie、そうでなければKnockBack
	if (owner->IsFloor())
	{
		if (owner->m_isDieOut)
		{
			owner->m_isDead = true;
			owner->ChangeState(std::make_shared<EnemyDie>(owner));
		}

		owner->m_rb.m_vel.y = 0.0f;

		//地面についたらknockDown状態に入る//当たり判定がでて吹き飛ぶので、そちらで対応
		owner->ChangeState(std::make_shared<EnemyKnockDown>(owner));
	}
}

void EnemyHitDrop::Exit()
{
}

void EnemyHitDrop::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:HitDrop");
}
