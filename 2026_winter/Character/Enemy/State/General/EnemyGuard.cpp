#include "EnemyGuard.h"
#include "../../../System.h"
#include "../../EnemyBase.h"

EnemyGuard::EnemyGuard(std::weak_ptr<EnemyBase> owner):
	EnemyStateBase(owner)
{
}

EnemyGuard::~EnemyGuard()
{
}

void EnemyGuard::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
}

void EnemyGuard::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);
}

void EnemyGuard::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
}

void EnemyGuard::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:Guard");
}
