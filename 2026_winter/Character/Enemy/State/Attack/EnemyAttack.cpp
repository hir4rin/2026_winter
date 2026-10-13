#include "EnemyAttack.h"
#include "../../EnemyBase.h"
#include "Character/Player/Player.h"
#include "Collision/AttackCol.h"
#include "Collision/SphereShape.h"
#include "Collision/CollisionManager.h"
#include <format>

namespace
{
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


	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);


	//animationの初期化
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(node.animName), false,node.animTimeScale,node.animEndFrame,0.0f,node.blendFrame);
	//向きはEnemyAttackPreで固定済みなので、ここではプレイヤーの方を向き直さない(向き直すと判定が避けた先に追いかけてくる)

	//攻撃の当たり判定を生成する
	CharacterBase::AttackData attackData = {
	.attackPower = node.attackPower,
	.brokenRate = node.brokenRate,
	.knockBackPower = Vector3(node.knockBackXZ,node.knockBackY,0.0f),
	.knockBackFrame = owner->m_anim.GetAnimTotalFrame(owner->GetAnimName("Attack")),//ここはどういう値がいいかかなり不明
	.hitStopTime = node.hitStopTime,
	.kAttackColOffset = 0.0f,
	.isKirimomi = node.isKirimomi
	};
	m_attackCol = std::make_shared<AttackCol>(owner, attackData);
	m_attackCol->ColInit({
		.pos = owner->m_rb.m_pos,
		.offset = owner->GetTargetVec() * node.colForwardOffset + Vector3(0, node.colOffsetY, 0),//攻撃中は向きが変わらないので、生成時の前方向で固定してよい
		.shape = std::make_unique<SphereShape>(node.colRadius),
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

	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);


	AttackMove();

	if (owner->m_anim.GetNowAnimFrame() >= node.attackFinishFrame)
	{
		owner->m_attackCoolTime = node.coolTime;//攻撃のクールタイムをリセット
		owner->ChangeState(owner->NextAfterAttack());
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

std::string EnemyAttack::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return {};
	const char* colText = (m_attackCol && m_attackCol->GetIsActive()) ? " 判定ON" : "";
	return std::format("rate {:.2f}{}", owner->m_anim.GetAnimRate(), colText);
}

void EnemyAttack::AttackMove()
{

	auto owner = m_owner.lock();
	if (!owner)return;
	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);

	float currentFrame = owner->m_anim.GetNowAnimFrame();
	Vector3 forward = owner->m_targetPos - owner->m_rb.m_pos;
	forward.y = 0.0f;
	forward = forward.Normalize();

	//移動距離
	if (currentFrame < node.attackColStartFrame)
	{
		owner->m_rb.m_vel = forward * node.moveSpeedBeforeCol;
	}
	else if (owner->m_anim.IsAnimFrameBetween(node.attackColStartFrame,node.attackColEndFrame))
	{
		owner->m_rb.m_vel = forward * node.moveSpeedDuringCol;
		if (m_attackCol) m_attackCol->SetIsActive(true);//攻撃の当たり判定を有効にする
	}

	else
	{
		owner->m_rb.m_vel = Vector3(0, 0, 0);
		if (m_attackCol) m_attackCol->SetIsActive(false);//攻撃の当たり判定を無効にする
	}
}
