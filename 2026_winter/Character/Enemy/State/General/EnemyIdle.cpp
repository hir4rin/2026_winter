#include "EnemyIdle.h"
#include "../../EnemyBase.h"

EnemyIdle::EnemyIdle(std::weak_ptr<EnemyBase> owner):
	EnemyStateBase(owner)
{
}

EnemyIdle::~EnemyIdle()
{
}

void EnemyIdle::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//owner->m_anim.Init();

	
}

void EnemyIdle::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//Playerを見る
	owner->ToPlayerLook();
	//m_idleTime += 1.0f * timeScale * m_ownTimeScale;
	//一定時間Idle状態でいる


	//if (m_idleTime < kEnemyIdleMaxTime)return;

	////ランダムでChaseかCautionに遷移する
	//if (owner->CanMeleeAttack(kEnemyMeleeAttackRange))
	//{
	//	owner->ChangeState(EnemyState::Attack);
	//	return;
	//}
	////ランダム
	//if (rand() % kStateChangeRandomMax < kStateChangeThreshold)
	//{
	//	owner->ChangeState(EnemyState::Chase);
	//	return;
	//}
	//else if (rand() % kStateChangeRandomMax >= kStateChangeThreshold)
	//{
	//	owner->ChangeState(EnemyState::Caution);
	//	return;
	//}

}

void EnemyIdle::Exit()
{
}
