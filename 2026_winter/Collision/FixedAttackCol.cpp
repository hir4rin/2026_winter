#include "FixedAttackCol.h"

FixedAttackCol::FixedAttackCol(std::weak_ptr<CharacterBase> owner, const CharacterBase::AttackData& data, const Vector3& pos)
	: AttackCol(owner, data), m_fixedPos(pos)
{
}

FixedAttackCol::~FixedAttackCol()
{
}

void FixedAttackCol::ApplyPos()
{
	m_rb.m_pos = m_fixedPos;
}
