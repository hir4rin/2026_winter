#include "EnemyAttackEnd.h"
#include "../../EnemyBase.h"
#include "Main/System.h"
#include <format>

namespace
{
}

EnemyAttackEnd::EnemyAttackEnd(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyAttackEnd::~EnemyAttackEnd()
{
}

void EnemyAttackEnd::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//アニメーションは変えない//攻撃のアニメーションはループしないので、最後の姿勢のまま止まる
	//その場で止まる
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	//頭もプレイヤーの方に向けない//隙を見せる
	owner->m_isLookAtPlayer = false;
	m_timer = 0.0f;
}

void EnemyAttackEnd::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);


	//向きも変えずにその場で止まる//押し戻しの処理が続かないように毎フレーム消す
	owner->m_rb.m_vel = Vector3(0, 0, 0);

	m_timer += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	if (m_timer >= node.attackEndTimeFrame)
	{
		owner->ChangeState(owner->NextAfterAttackEnd());
		return;
	}
}

void EnemyAttackEnd::Exit()
{
}

std::string EnemyAttackEnd::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return{};
	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);

	return std::format("{:.0f}/{:.0f}F", m_timer, node.attackEndTimeFrame);
}
