#include "HitCol.h"
#include "CharacterBase.h"

HitCol::HitCol(std::weak_ptr<CharacterBase> owner)
	: m_owner(owner)
{
	//やられ判定の初期化をOwnerがする

}

HitCol::~HitCol()
{
}

void HitCol::OnCollision(Collider& other)
{
	//何もしない
}

//void HitCol::OnDamageInterFace(Collider& other, CharacterBase::AttackData& data)
//{
//	//所有者にデータを渡す
//	auto owner = m_owner.lock();
//	if (!owner)return;
//
//	owner->OnDamage(other, data);
//}

void HitCol::SetTimeScaleInterFace(float timeScale, float time)
{
	//所有者にデータを渡す
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->SetOwnTimeScale(timeScale,time);
}

void HitCol::ApplyPos()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//座標の更新
	m_rb.m_pos = owner->GetRigidBody().GetPos();

	////m_pos = owner->GetPos();
}
