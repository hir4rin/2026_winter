#include "EnemyAttackPre.h"
#include "../../EnemyBase.h"
#include "Character/Player/Player.h"
#include "Main/System.h"
#include <format>

namespace
{
	constexpr float kAttackPreBlendRate = 0.5f;
	constexpr float kAttackPreTrackEndRate = 0.6f;//予備動作:プレイヤーの方を向き続けるrateの上限(これ以降は向きを固定して避けられるようにする



	
}

EnemyAttackPre::EnemyAttackPre(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyAttackPre::~EnemyAttackPre()
{
}

void EnemyAttackPre::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;

	//m_currentAttackIndexはここに来る前(NextAfterIdleなど)で決めてある
	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);

	//animationの初期化//空欄なら予備動作のアニメを流さない
	if (!node.preAnimName.empty())
	{
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(node.preAnimName), false, node.preAnimTimeScale, node.preAnimEndFrame, 0.0f, node.preBlendFrame);
	}
	
	//その場で止まる
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	//Playerを見る
	owner->m_targetPos = player->GetRigidBody().GetPos();
	owner->ToPlayerLook();

	//頭をプレイヤーの方に向ける
	owner->m_isLookAtPlayer = true;
}

void EnemyAttackPre::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;

	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);

	owner->m_rb.m_vel = Vector3(0, 0, 0);

	float timescale = System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	m_timer += 1.0f * timescale;



	//予備動作の前半はプレイヤーの方を向き続ける
	//EnemyAttackはm_targetPosに向かって踏み込むので、ここで更新しておく
	if (m_timer <= node.preLookTime)
	{
		owner->m_targetPos = player->GetRigidBody().GetPos();
		owner->ToPlayerLook();
	}

	//予備動作が終わったら攻撃
	if (m_timer  >= node.preTime)
	{
		owner->ChangeState(std::make_shared<EnemyAttack>(owner));
		return;
	}
}

void EnemyAttackPre::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//頭の向きを正面に戻す
	owner->m_isLookAtPlayer = false;
}

void EnemyAttackPre::DebugDraw()
{
}

std::string EnemyAttackPre::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return{};
	const auto& node = owner->GetAttackNode(owner->m_currentAttackIndex);


	//向きを固定した後は「固定」を付ける(見てから避けられる時間に入ったか確認する用)
	const char* lockText = (m_timer > node.preLookTime) ? " 固定" : "";
	return std::format("{:.0f}/{:.0f}F{}", m_timer, node.preTime, lockText);
}
