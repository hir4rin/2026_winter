#include "EnemyAirFall.h"
#include "../../EnemyBase.h"
#include "../../../System.h"
#include "../Game.h"

EnemyAirFall::EnemyAirFall(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyAirFall::~EnemyAirFall()
{
}

void EnemyAirFall::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下用のアニメーションが用意できたらここで再生する
	//静止状態から落ち始める
	owner->m_accumulatedGravity = 0.0f;
}

void EnemyAirFall::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);

	//水平方向は止めて、重力だけかける//累積から毎フレーム作り直す(タイムスケールで減衰しないように)
	owner->m_accumulatedGravity += -Game::kGravity * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	owner->m_rb.m_vel = Vector3(0, owner->m_accumulatedGravity, 0);

	//着地したらIdleに戻る
	if (owner->IsFloor())
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
	}
}

void EnemyAirFall::Exit()
{
}

void EnemyAirFall::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:AirFall");
}
