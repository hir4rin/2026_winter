#include "EnemyDie.h"
#include "../../EnemyBase.h"

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
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Death"), false);
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	owner->m_isLifeZero = true;
	//当たり判定の解除(旧Terminate相当)は、EnemyBase側に処理を用意したらここで呼ぶ
}

void EnemyDie::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);
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
}

void EnemyDie::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:Die");
}
