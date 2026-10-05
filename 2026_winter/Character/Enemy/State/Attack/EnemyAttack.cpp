#include "EnemyAttack.h"
#include "../../EnemyBase.h"
#include "Player.h"
#include "../../../AttackCol.h"
#include "../../../../Collider/SphereShape.h"
#include "../../../../Managers/CollisionManager.h"

namespace
{
	constexpr float kAttackPower = 10.0f;//攻撃力
	constexpr float kAttackColRadius = 80.0f;//攻撃の当たり判定の半径
	constexpr float kAttackColOffsetY = 100.0f;//攻撃の当たり判定の高さ//前方へのオフセットはAttackCol::ApplyPosで行う
	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間

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

	//攻撃の当たり判定を生成する
	CharacterBase::AttackData attackData = {
	.attackPower = kAttackPower,
	.brokenRate = 0.0f,
	.knockBackPower = Vector3(0, 0, 0),
	.knockBackFrame = owner->m_anim.GetAnimTotalFrame(owner->GetAnimName("Attack")),
	.hitStopTime = kHitStopTime,
	.kAttackColOffset = 0.0f,
	.isKirimomi = false
	};
	m_attackCol = std::make_shared<AttackCol>(owner, attackData);
	m_attackCol->ColInit({
		.pos = owner->m_rb.m_pos,
		.offset = Vector3(0, kAttackColOffsetY, 0),
		.shape = std::make_unique<SphereShape>(kAttackColRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::Attack},
		.isActive = false,
		.isTrigger = true
		});//最初は無効にしておく
	m_attackCol->SetIsActive(false);
}

void EnemyAttack::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	AttackMove();

	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->m_attackCoolTime = kEnemyAttackCoolTime;//攻撃のクールタイムをリセット
		owner->ChangeState(std::make_shared<EnemyBack>(owner));
	}
}

void EnemyAttack::Exit()
{
	//攻撃の当たり判定を削除する
	if (m_attackCol)
	{
		CollisionManager::GetInstance().ReleaseCollider(m_attackCol);
		m_attackCol->SetIsActive(false);
		m_attackCol->SetLifeTimeLimited();
		m_attackCol.reset();
	}
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
		if (m_attackCol) m_attackCol->SetIsActive(true);//攻撃の当たり判定を有効にする
	}

	else
	{
		owner->m_rb.m_vel = Vector3(0, 0, 0);
		if (m_attackCol) m_attackCol->SetIsActive(false);//攻撃の当たり判定を無効にする
	}
}
