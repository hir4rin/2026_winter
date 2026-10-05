#include "JustDodgeCol.h"
#include "Player.h"

JustDodgeCol::JustDodgeCol(std::weak_ptr<Player> owner)
	: m_owner(owner)
{
	//当たり判定の初期化はPlayerStateDodgeがする
}

JustDodgeCol::~JustDodgeCol()
{
}

void JustDodgeCol::OnCollision(Collider& other)
{
	//何もしない
}

void JustDodgeCol::OnJustDodgeInterFace(Collider& other, CharacterBase::AttackData& data)
{
	//所有者に通知する
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->OnJustDodge(other, data);
}

void JustDodgeCol::ApplyPos()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//座標の更新
	m_rb.m_pos = owner->GetRigidBody().GetPos();
}
