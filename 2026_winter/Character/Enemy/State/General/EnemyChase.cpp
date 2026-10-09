#include "EnemyChase.h"
#include "../../../System.h"
#include "../../EnemyBase.h"
#include "Player.h"

namespace
{
	constexpr float kEnemyMeleeAttackRange = 400.0f;//敵の近接攻撃の距離
	constexpr float kEnemyTargetUpdateTime = 30.0f;//敵がターゲットを更新する時間

}

EnemyChase::EnemyChase(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyChase::~EnemyChase()
{
}

void EnemyChase::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;
	//animationの初期化
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Run"), true);
	owner->m_targetPos = player->GetRigidBody().GetPos();
	//Playerを見る
	owner->ToPlayerLook();

	//時間をリセット
	owner->m_chasingTime = 0.0f;

	//頭をプレイヤーの方に向ける
	owner->m_isLookAtPlayer = true;
}

void EnemyChase::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//攻撃可能な距離に入ったら攻撃
	if (owner->CanMeleeAttack(kEnemyMeleeAttackRange))
	{
		//ChangeState(EnemyState::Attack);
		owner->ChangeState(std::make_shared<EnemyAttack>(owner));
		return;
	}

	//定期的にプレイヤーの位置を更新する
	if (!owner->ChaseTarget(owner->m_targetPos, kEnemyMeleeAttackRange))
	{
		//後ほど
		//owner->m_chasingTime += 1.0f * timeScale * m_ownTimeScale;
		owner->m_chasingTime += 1.0f * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
		//更新
		if (owner->m_chasingTime > kEnemyTargetUpdateTime)
		{
			owner->m_chasingTime = 0.0f;
			owner->m_targetPos = owner->TargetPlayerPos();
			//Playerを見る
			owner->ToPlayerLook();
		}
	}
	//playerの位置についたら
	else
	{
		//いろいろやってもなぜか攻撃せず止まるので、距離関係なく攻撃させる
		//ChangeState(EnemyState::Attack);

		owner->ChangeState(std::make_shared<EnemyAttack>(owner));
		return;
	}
}

void EnemyChase::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//頭の向きを正面に戻す
	owner->m_isLookAtPlayer = false;
}

void EnemyChase::DebugDraw()
{
}
