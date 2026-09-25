#include "EnemyAttack.h"
#include "../../EnemyBase.h"
#include "Player.h"

namespace
{
	constexpr float kEnemyAttackCoolTime = 120.0f;//敵の攻撃のクールタイム
	constexpr float kAttackMoveStartRate = 0.3f;//攻撃モーション:移動を開始するrate
	constexpr float kAttackColActivateRate = 0.5f;//攻撃モーション:攻撃判定を有効にするrateの上限
	constexpr float kAttackMoveSpeed1 = 10.0f;//攻撃モーション前半の移動速度
	constexpr float kAttackMoveSpeed2 = 16.0f;//攻撃モーション後半の移動速度
}

EnemyAttack::EnemyAttack(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyAttack::~EnemyAttack()
{
}

void EnemyAttack::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;
	//animationの初期化
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Attack"), false);
	//Playerを見る
	owner->ToPlayerLook();
}

void EnemyAttack::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	AttackMove();

	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->m_attackCoolTime = kEnemyAttackCoolTime;//攻撃のクールタイムをリセット
		//owner->ChangeState(EnemyState::Back);//BackStateに移行
		owner->ChangeState(std::make_shared<EnemyBack>(owner));
	}
}

void EnemyAttack::Exit()
{
	//m_attackCol->SetIsActive(false);
	//m_attackCol->ClearHitIds();
}

void EnemyAttack::DebugDraw()
{
}

void EnemyAttack::AttackMove()
{

	auto owner = m_owner.lock();
	if (!owner)return;

	float rate = owner->m_anim.GetAnimRate();
	Vector3 forward = owner->m_targetPos - owner->m_rb.m_pos;
	forward.y = 0.0f;
	forward = forward.Normalize();
	//移動距離
	if (rate <= kAttackMoveStartRate)
	{
		owner->m_rb.m_vel = forward * kAttackMoveSpeed1;
	}
	else if (rate > kAttackMoveStartRate && rate <= kAttackColActivateRate)
	{
		owner->m_rb.m_vel = forward * kAttackMoveSpeed2;
		//owner->m_attackCol->SetIsActive(true);
	}

	else
	{
		owner->m_rb.m_vel = Vector3(0, 0, 0);
	}
}
