#include "EnemyIdle.h"
#include "Main/System.h"
#include "../../EnemyBase.h"
#include <format>

namespace
{
}

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

	//時間をリセット
	owner->m_idleTime = 0.0f;

	owner->m_isLookAtPlayer = true;
}

void EnemyIdle::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//押し戻しの処理が続かないように消す//落下中の速度は残す
	owner->m_rb.m_vel = Vector3(0, owner->m_rb.m_vel.y, 0);
	//owner->m_rb.m_vel = Vector3(0, 0, 0);//EnemyStateFallを作ったらこっちに移行

	//Playerを見る
	//owner->ToPlayerLook();

	//m_idleTime += 1.0f * timeScale * m_ownTimeScale;
	owner->m_idleTime += 1.0f * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	

	//一定時間Idle状態でいる

	//デバッグ用//Idleのままにするときは遷移しない
	if (owner->m_isDebugIdle)return;

	if (owner->m_idleTime < owner->m_status.idleTime)return;

	//Idleの時間を超えたら次のStateに遷移する
	owner->ChangeState(owner->NextAfterIdle());
}

void EnemyIdle::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_isLookAtPlayer = false;
}

void EnemyIdle::DebugDraw()
{
}

std::string EnemyIdle::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return {};
	return std::format("{:.0f}/{:.0f}F", owner->m_idleTime, owner->m_status.idleTime);
}
