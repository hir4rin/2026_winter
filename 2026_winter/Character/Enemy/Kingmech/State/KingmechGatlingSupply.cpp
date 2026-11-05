#include "KingmechGatlingSupply.h"
#include "Character/Enemy/Kingmech/BossEnemyKingmech.h"
#include "Character/Enemy/Kingmech/Part/KingmechGatling.h"
#include "Main/System.h"

namespace
{
	const std::string kCallAnim = "Roar";//補給を呼ぶ
	const std::string kRecoverAnim = "Idle";//後隙

	constexpr float kCallTime = 30.0f;//呼んでから落ち始めるまで
	constexpr float kRecoverTime = 60.0f;//後隙の長さ
}

KingmechGatlingSupply::KingmechGatlingSupply(std::weak_ptr<EnemyBase> owner) :
	EnemyStateBase(owner)
{
}

KingmechGatlingSupply::~KingmechGatlingSupply()
{
}

void KingmechGatlingSupply::Enter()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	ChangePhase(Phase::Call);
}

void KingmechGatlingSupply::Update()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	m_frame += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	switch (m_phase)
	{
	case Phase::Call:
		owner->ToPlayerLook();
		if (m_frame >= kCallTime)
		{
			owner->m_gatling->StartSupply();
			ChangePhase(Phase::Drop);
		}
		break;
	case Phase::Drop:
		//付いたら後隙
		if (owner->m_gatling->GetPhase() != KingmechGatling::Phase::Supply)
		{
			ChangePhase(Phase::Recover);
		}
		break;
	case Phase::Recover:
		if (m_frame >= kRecoverTime)
		{
			owner->ChangeState(owner->NextAfterRecover());
		}
		break;
	}
}

void KingmechGatlingSupply::Exit()
{
}

void KingmechGatlingSupply::ChangePhase(Phase next)
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	m_phase = next;
	m_frame = 0.0f;

	switch (m_phase)
	{
	case Phase::Call:
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kCallAnim), true, owner->GetAnimSpeed(kCallAnim));
		break;
	case Phase::Drop:
		break;
	case Phase::Recover:
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kRecoverAnim), true, owner->GetAnimSpeed(kRecoverAnim));
		break;
	}
}
