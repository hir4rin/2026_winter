#include "EnemyKnockDown.h"
#include "../../EnemyBase.h"
#include "../../../System.h"

namespace
{
}

EnemyKnockDown::EnemyKnockDown(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyKnockDown::~EnemyKnockDown()
{
}

void EnemyKnockDown::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	//ダウン用のアニメーションが用意できたらここで再生する
}

void EnemyKnockDown::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	m_downTime += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	//一定時間ダウンしたら立ち直った後の行動へ(初期はIdle)
	if (m_downTime > owner->m_status.knockDownTime)
	{
		owner->ChangeState(owner->NextAfterRecover());
	}
}

void EnemyKnockDown::Exit()
{
}

void EnemyKnockDown::DebugDraw()
{
}
