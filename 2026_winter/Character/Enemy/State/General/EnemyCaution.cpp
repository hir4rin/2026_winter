#include "EnemyCaution.h"
#include "../../../System.h"
#include "Player.h"
#include "../../EnemyBase.h"

namespace
{
	constexpr float kEnemyCautionMaxTime = 60.0f;//敵が警戒する時間の最大値
	constexpr float kEnemyCautionUpdateIntervalDivisor = 3.0f;//警戒中の位置更新間隔の分母
	constexpr float kEnemyMeleeAttackRange = 400.0f;//敵の近接攻撃の距離

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
	if (owner->m_cautionTime > kEnemyCautionMaxTime)
	{
		owner->m_cautionTime = 0.0f;
		owner->ChangeState(std::make_shared<EnemyChase>(owner));
		return;
	}
	//定期的にプレイヤーの位置を更新する
	if (owner->CountInterval(owner->m_cautionUpdateTimer, kEnemyCautionMaxTime / kEnemyCautionUpdateIntervalDivisor))
	{
		owner->m_targetPos = owner->TargetPlayerPos();
	}
	owner->CautionMove(owner->m_targetPos, kEnemyMeleeAttackRange);
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
