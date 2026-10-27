#include "BossGuardBreak.h"
#include "Character/Enemy/BossPirate/BossEnemyPirateR1.h"
#include "Character/Enemy/BossPirate/Part/BossArm.h"

BossGuardBreak::BossGuardBreak(std::weak_ptr<EnemyBase> owner) :
	EnemyGuardBreak(owner)
{
}

BossGuardBreak::~BossGuardBreak()
{
}

void BossGuardBreak::Enter()
{
	EnemyGuardBreak::Enter();

	auto boss = std::dynamic_pointer_cast<BossEnemyPirateR1>(m_owner.lock());
	if (!boss)return;
	//腕を切り離す
	if (auto arm = boss->GetLeftBackArm())arm->Detach();
	if (auto arm = boss->GetRightBackArm())arm->Detach();
}

void BossGuardBreak::Exit()
{
	EnemyGuardBreak::Exit();

	//立ち直ったら腕を戻す
	auto boss = std::dynamic_pointer_cast<BossEnemyPirateR1>(m_owner.lock());
	if (!boss)return;
	if (auto arm = boss->GetLeftBackArm())arm->RequestReturn();
	if (auto arm = boss->GetRightBackArm())arm->RequestReturn();
}
