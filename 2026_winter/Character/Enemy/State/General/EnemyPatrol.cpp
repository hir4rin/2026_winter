#include "EnemyPatrol.h"
#include "../../../System.h"
#include "Player.h"
#include "../../EnemyBase.h"

namespace
{
}

EnemyPatrol::EnemyPatrol(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyPatrol::~EnemyPatrol()
{
}

void EnemyPatrol::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
}

void EnemyPatrol::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;



}

void EnemyPatrol::Exit()
{
}

void EnemyPatrol::DebugDraw()
{
}
