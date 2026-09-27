#include "EnemyAssasined.h"
#include "../../EnemyBase.h"
#include "../../HitCol.h"


EnemyAssasined::EnemyAssasined(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyAssasined::~EnemyAssasined()
{
}

void EnemyAssasined::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//被暗殺アニメーション
	//	Execute01Victim
	//	Execute02Victim
	//	Execute03Victim
	//	FootPlantVictim
	//	RunSlashFinisher
	//	StabBehindVictim
	//	StabChestVictim
	//	fromAvobeVictim
	//	fromAvobeForwardVictim
	//	SneakStabBehindVictim



	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Execute02Victim"), false,0.7f);


	//押し戻しを無効化
	owner->SetIsActive(false);
}

void EnemyAssasined::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;



	//今はテスト中だから終わったらIdleに戻す
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
	}

	
}

void EnemyAssasined::Exit()
{
}

void EnemyAssasined::DebugDraw()
{
}
