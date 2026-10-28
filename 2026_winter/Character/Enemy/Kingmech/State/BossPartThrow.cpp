#include "BossPartThrow.h"
#include "Character/Enemy/Kingmech/BossEnemyKingmech.h"
#include "Main/System.h"

namespace
{
	//ここは後ほどcsvのほうで修正
	const std::string kKneelStartAnim = "KnockStart";//膝をつく
	const std::string kKneelLoopAnim = "KnockLoop";
	const std::string kStandUpAnim = "KnockEnd";//立ち上がる

	constexpr float kMaxWaitFrame = 600.0f;//これだけ待っても投げられなければボスが自分で投げさせる
}

BossPartThrow::BossPartThrow(std::weak_ptr<EnemyBase> owner) :
	EnemyStateBase(owner)
{
}

BossPartThrow::~BossPartThrow()
{
}

void BossPartThrow::Enter()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kKneelStartAnim), false, owner->GetAnimSpeed(kKneelStartAnim));
}

void BossPartThrow::Update()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	m_frame += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	//立ち上がり終わったら次の行動へ
	if (m_isExploded)
	{
		if (owner->m_anim.GetAnimEndFlag())
		{
			owner->ChangeState(owner->NextAfterRecover());
		}
		return;
	}

	//膝をついたらそのまま待つ
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kKneelLoopAnim), true, owner->GetAnimSpeed(kKneelLoopAnim));
	}

	//プレイヤーが投げられなかったとき(被弾などで)
	if (m_frame >= kMaxWaitFrame)
	{
		owner->ThrowPart();
	}
}

void BossPartThrow::Exit()
{
}

void BossPartThrow::OnExploded()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	m_isExploded = true;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kStandUpAnim), false, owner->GetAnimSpeed(kStandUpAnim));
}
