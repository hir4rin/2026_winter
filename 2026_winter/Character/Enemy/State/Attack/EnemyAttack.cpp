#include "EnemyAttack.h"
#include "../../EnemyBase.h"
#include "Character/Player/Player.h"
#include "Collision/AttackCol.h"
#include "Collision/SphereShape.h"
#include "Collision/CollisionManager.h"
#include <format>

namespace
{
	constexpr float kChainForwardDistance = 1000.0f;//連撃で向き直さない敵の踏み込み先(今の向きの前方)
}

EnemyAttack::EnemyAttack(std::weak_ptr<EnemyBase> owner, bool isChain):EnemyStateBase(owner),
	m_isChain(isChain)
{
}

EnemyAttack::~EnemyAttack()
{
}

void EnemyAttack::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下禁止を有効化
	owner->SetPreventFall(true);
	auto player = owner->m_player.lock();
	if (!player)return;


	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);

	//連撃の2撃目以降は踏み込み先を決め直す//判定の位置はこの後の向きで決まるので、先にする
	if (m_isChain)
	{
		if (owner->IsChainAttackLookAtPlayer())
		{
			//プレイヤーの方を向いて、プレイヤーの位置へ踏み込む
			owner->m_targetPos = player->GetRigidBody().GetPos();
			owner->ToPlayerLook();
		}
		else
		{
			//向きはそのまま//1撃目で通り過ぎた踏み込み先へ戻らないように、前方に置き直す
			owner->m_targetPos = owner->m_rb.m_pos + owner->GetTargetVec() * kChainForwardDistance;
		}
	}

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

	//attackFinishFrameが負(空欄)ならアニメの終わりまで
	const bool isFinish = (node.attackFinishFrame < 0.0f)
		? owner->m_anim.GetAnimEndFlag()
		: owner->m_anim.GetNowAnimFrame() >= node.attackFinishFrame;
	if (isFinish)
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
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下禁止を無効化
	owner->SetPreventFall(false);
}

void EnemyAttack::DebugDraw()
{
}

std::string EnemyAttack::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return {};
	const char* colText = (m_attackCol && m_attackCol->GetIsActive()) ? " 判定ON" : "";
	const char* chainText = m_isChain ? " 連" : "";
	return std::format("#{}{} rate {:.2f}{}", owner->m_currentAttackIndex, chainText, owner->m_anim.GetAnimRate(), colText);
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
