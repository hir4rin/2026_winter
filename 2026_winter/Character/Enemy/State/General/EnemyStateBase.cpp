#include "EnemyStateBase.h"

EnemyStateBase::EnemyStateBase(std::weak_ptr<EnemyBase> owner) :
	m_owner(owner)
{
}
