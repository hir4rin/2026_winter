#include "AttackTokenManager.h"
#include "../Character/Enemy/EnemyBase.h"
#include "Main/System.h"
#include <algorithm>


namespace
{
    constexpr float kGlobalAttackInterval = 30.0f;//攻撃権を渡す間隔(同時に斬りかからないように)
}

bool AttackTokenManager::Request(EnemyBase& enemy, int cost)
{
	//すでに持っている
	if (HasToken(enemy))return true;
	if (!CanRequest(cost))return false;

	m_holders.push_back({ enemy.GetWeakPtr(), cost });
	m_globalInterval = kGlobalAttackInterval;
	return true;
}

void AttackTokenManager::Release(const EnemyBase& enemy)
{


	//weak_ptr.lock().get()でアドレスを取れるからそれで比較でもよい
	std::erase_if(m_holders, [&enemy](const Holder& holder)
		{
			const auto holderEnemy = holder.holder.lock();
			return holderEnemy && holderEnemy->GetId() == enemy.GetId();
		});
}

bool AttackTokenManager::HasToken(const EnemyBase& enemy) const
{

	//std::any_ofはリストの中に条件に合うものが一つでもいるかどうか//for文よりも見つかった時点で調べるのをやめるので、いい
	return std::any_of(m_holders.begin(), m_holders.end(), [&enemy](const Holder& holder)
		{
			const auto holderEnemy = holder.holder.lock();
			return holderEnemy && holderEnemy->GetId() == enemy.GetId();
		});
}

bool AttackTokenManager::CanRequest(int cost) const
{
	//攻撃間隔中
	if (m_globalInterval > 0.0f)return false;
	//重さの上限を超える
	if (GetUsedCost() + cost > m_maxCost)return false;
	return true;
}

void AttackTokenManager::AllClear()
{
	m_holders.clear();
	m_globalInterval = 0.0f;
}

void AttackTokenManager::Update()
{
	//スローに合わせて減らす
	if (m_globalInterval > 0.0f)
	{
		m_globalInterval -= System::GetInstance().GetTimeScale();
	}

	//消えた敵・死んだ敵の攻撃権を回収(返し忘れ対策)
	std::erase_if(m_holders, [](const Holder& holder)
		{
			const auto enemy = holder.holder.lock();
			return !enemy || enemy->GetIsLifeZero();
		});
}

int AttackTokenManager::GetUsedCost() const
{
	int used = 0;
	for (const auto& holder : m_holders)
	{
		used += holder.cost;
	}
	return used;
}
