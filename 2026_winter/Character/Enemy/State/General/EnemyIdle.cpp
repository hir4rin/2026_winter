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
	//animationの初期化
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Idle"), true);
	//移動速度を0にする
	owner->m_rb.m_vel = Vector3(0, 0, 0);
}

void EnemyIdle::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//押し戻しの処理が続かないように消す//落下中の速度は残す
	owner->m_rb.m_vel = Vector3(0, owner->m_rb.m_vel.y, 0);

	//Playerを見る
	owner->ToPlayerLook();

	owner->m_anim.Update(owner->m_ownTimeScale);
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

void EnemyIdle::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:Idle");
}
