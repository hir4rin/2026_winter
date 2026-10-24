#include "EnemyDie.h"
#include "../../EnemyBase.h"
#include "Collision/HitCol.h"

EnemyDie::EnemyDie(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyDie::~EnemyDie()
{
}

void EnemyDie::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Death"), false, owner->GetAnimSpeed("Death"));
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	owner->m_isLifeZero = true;
	//やられ判定を消す(死体に攻撃が当たらないようにする)
	if (owner->m_hitCol) owner->m_hitCol->SetIsActive(false);

	//キャラ同士の押し戻しを無効化
	owner->SetIsGhost(true);


}

void EnemyDie::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//水平方向は止める//縦の重力はEnemySwordman::Updateがかけている
	owner->m_rb.m_vel = Vector3(0, owner->m_rb.m_vel.y, 0);

	//死亡アニメーションが終わったら消えてよい状態にする
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->m_isDead = true;
	}
}

void EnemyDie::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//キャラ同士の押し戻しを有効化
	owner->SetIsGhost(false);

}

void EnemyDie::DebugDraw()
{
}
