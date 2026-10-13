#include "EnemyKnockBack.h"
#include "../../EnemyBase.h"
#include "Main/System.h"

namespace
{
}

EnemyKnockBack::EnemyKnockBack(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyKnockBack::~EnemyKnockBack()
{
}

void EnemyKnockBack::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	//ダウン用のアニメーションが用意できたらここで再生する
}

void EnemyKnockBack::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	m_downTime += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	//一定時間ダウンしたら立ち直った後の行動へ(初期はIdle)
	if (m_downTime > owner->m_status.knockBackTime)
	{
		owner->ChangeState(owner->NextAfterRecover());
	}
}

void EnemyKnockBack::Exit()
{
}

void EnemyKnockBack::DebugDraw()
{
}
