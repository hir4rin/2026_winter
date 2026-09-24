#include "EnemyChase.h"
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
}

void EnemyChase::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//攻撃可能な距離に入ったら攻撃
	if (owner->CanMeleeAttack(kEnemyMeleeAttackRange))
	{
		//ChangeState(EnemyState::Attack);
	}

	//定期的にプレイヤーの位置を更新する
	if (!owner->ChasePlayer(owner->m_targetPos, kEnemyMeleeAttackRange))
	{
		//後ほど
		//owner->m_chasingTime += 1.0f * timeScale * m_ownTimeScale;
		owner->m_chasingTime += 1.0f;
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
}

void EnemyChase::DebugDraw()
{
}
