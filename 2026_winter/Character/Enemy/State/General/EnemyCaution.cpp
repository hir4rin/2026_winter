#include "EnemyCaution.h"
#include "../../../System.h"
#include "Player.h"
#include "../../EnemyBase.h"
#include <format>

namespace
{
	constexpr float kEnemyCautionUpdateIntervalDivisor = 3.0f;//警戒中の位置更新間隔の分母
}

EnemyCaution::EnemyCaution(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyCaution::~EnemyCaution()
{
}

void EnemyCaution::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;

	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("StrafeRight"), true);

	owner->m_targetPos = player->GetRigidBody().GetPos();

	//Playerを見る
	owner->ToPlayerLook();

	owner->m_cautionTime = 0.0f;

	//頭をプレイヤーの方に向ける
	owner->m_isLookAtPlayer = true;
}

void EnemyCaution::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;

	//Playerを見る
	owner->ToPlayerLook();

	//一定時間様子を見る
	//その後、Chaseに移行

	//owner->m_cautionTime += 1.0f * timeScale * m_ownTimeScale;
	owner->m_cautionTime += 1.0f * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	if (owner->m_cautionTime > owner->m_status.cautionTime)
	{
		owner->m_cautionTime = 0.0f;
		owner->ChangeState(owner->NextAfterCaution());
		return;
	}

	//定期的にプレイヤーの位置を更新する
	if (owner->CountInterval(owner->m_cautionUpdateTimer, owner->m_status.cautionTime / kEnemyCautionUpdateIntervalDivisor))
	{
		owner->m_targetPos = owner->TargetPlayerPos();
	}
	owner->CautionMove(owner->m_targetPos, owner->GetMaxAttackRange());
}

void EnemyCaution::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//頭の向きを正面に戻す
	owner->m_isLookAtPlayer = false;
}

void EnemyCaution::DebugDraw()
{
}

std::string EnemyCaution::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return {};
	return std::format("{:.0f}/{:.0f}F", owner->m_cautionTime, owner->m_status.cautionTime);
}
