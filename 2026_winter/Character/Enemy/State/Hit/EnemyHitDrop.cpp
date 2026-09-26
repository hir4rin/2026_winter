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
	//縦の初速を渡す//縦の速度は初速と重力の累積からEnemySwordman::Updateが作る
	owner->m_initVelY = m_info.knockBackVel.y;
	owner->m_accumulatedGravity = 0.0f;

	//地面にいない判定にする
	owner->SetIsFloor(false);
	owner->m_rb.m_vel = Vector3(m_info.knockBackVel.x, owner->m_rb.m_vel.y, m_info.knockBackVel.z);
}

void EnemyHitDrop::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);

	//水平方向は毎フレームHitInfoの初速を与える(EnemySwordman::Updateで毎フレーム水平速度がリセットされるため)
	owner->m_rb.m_vel = Vector3(m_info.knockBackVel.x, owner->m_rb.m_vel.y, m_info.knockBackVel.z);

	//着地したら、死亡予定ならDie、そうでなければKnockBack
	if (owner->IsFloor())
	{
		if (owner->m_isDieOut)
		{
			owner->m_isDead = true;
			owner->ChangeState(std::make_shared<EnemyDie>(owner));
			return;
		}

		owner->m_rb.m_vel.y = 0.0f;

		//地面についたらknockDown状態に入る//当たり判定がでて吹き飛ぶので、そちらで対応
		owner->ChangeState(std::make_shared<EnemyKnockDown>(owner));
	}
}

void EnemyHitDrop::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//初速を戻しておく(残っていると着地後も縦の速度がかかり続ける)
	owner->m_initVelY = 0.0f;
}

void EnemyHitDrop::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:HitDrop");
}
