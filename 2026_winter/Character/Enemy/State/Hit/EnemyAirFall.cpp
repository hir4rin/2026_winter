#include "EnemyAirFall.h"
#include "../../EnemyBase.h"
#include "Main/System.h"
#include "Main/Game.h"

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
	m_gravity = 0.0f;
}

void EnemyAirFall::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;


	//水平方向は止めて、重力だけかける//累積から毎フレーム作り直す(タイムスケールで減衰しないように)
	m_gravity += -Game::kGravity * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	owner->m_rb.m_vel = Vector3(0, m_gravity, 0);

	//着地したら立ち直った後の行動へ(初期はIdle)
	if (owner->IsFloor())
	{
		owner->ChangeState(owner->NextAfterRecover());
	}
}

void EnemyAirFall::Exit()
{
}

void EnemyAirFall::DebugDraw()
{
}
