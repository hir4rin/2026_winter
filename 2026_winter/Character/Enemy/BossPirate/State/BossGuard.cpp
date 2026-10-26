#include "BossGuard.h"
#include "Character/Enemy/BossPirate/BossEnemyPirateR1.h"
#include "Character/Enemy/BossPirate/Part/BossArm.h"
#include "BossGuardBreak.h"

namespace
{
	constexpr float kArmGuardAnimSpeed = 0.5f;//ボスのガードと同じ速さ
}

BossGuard::BossGuard(std::weak_ptr<EnemyBase> owner) :
	EnemyGuard(owner)
{
}

BossGuard::~BossGuard()
{
}

void BossGuard::Enter()
{
	EnemyGuard::Enter();

	auto boss = std::dynamic_pointer_cast<BossEnemyPirateR1>(m_owner.lock());
	if (!boss)return;
	if (auto arm = boss->GetLeftBackArm())arm->PlayAnim("BlockLoop", true, kArmGuardAnimSpeed);
	if (auto arm = boss->GetRightBackArm())arm->PlayAnim("BlockLoop", true, kArmGuardAnimSpeed);
}

void BossGuard::Exit()
{
	EnemyGuard::Exit();

	//腕を普段に戻す
	auto boss = std::dynamic_pointer_cast<BossEnemyPirateR1>(m_owner.lock());
	if (!boss)return;
	if (auto arm = boss->GetLeftBackArm())arm->ReturnToIdle();
	if (auto arm = boss->GetRightBackArm())arm->ReturnToIdle();
}

std::shared_ptr<EnemyStateBase> BossGuard::CreateGuardBreakState()
{
	return std::make_shared<BossGuardBreak>(m_owner);
}
