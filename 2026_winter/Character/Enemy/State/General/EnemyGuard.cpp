#include "EnemyGuard.h"
#include "../../../System.h"
#include "../../EnemyBase.h"
#include <format>



namespace
{
}

EnemyGuard::EnemyGuard(std::weak_ptr<EnemyBase> owner):
	EnemyStateBase(owner)
{
}

EnemyGuard::~EnemyGuard()
{
}

void EnemyGuard::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.ChangeAnim(owner->GetAnimName("BlockLoop"), true,0.5f);
}

void EnemyGuard::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	float timeScale = System::GetInstance().GetTimeScale();
	float& timer = owner->m_guardInfo.GuardTimer;

	timer += 1.0f * timeScale * owner->m_ownTimeScale;

	if (timer >= owner->m_status.guardTime)
	{
		owner->ChangeState(owner->NextAfterRecover());//ガードが時間切れで終わった
		return;
	}

	//ガード喰らい状態が終わったら、またアニメーションを元に戻す
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->m_anim.ChangeAnim(owner->GetAnimName("BlockLoop"), true,0.5f); 
	}
}

void EnemyGuard::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//ガード情報をリセット
	owner->m_guardInfo.GuardTimer = 0.0f;
	owner->m_guardInfo.GurardGauge = 0.0f;
}

void EnemyGuard::DebugDraw()
{
}

std::string EnemyGuard::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return {};
	//書式の文字列に日本語を入れると、Shift-JISの2バイト目が{や}と同じ値になることがあるので英字にしておく
	return std::format("gauge {:.0f}/{:.0f} {:.0f}/{:.0f}F",
		owner->m_guardInfo.GurardGauge, owner->m_status.parryGauge, owner->m_guardInfo.GuardTimer, owner->m_status.guardTime);
}

void EnemyGuard::OnGuardHit()
{
	//アニメーションを変更する処理//これが終わった後また戻さないといけない
	auto owner = m_owner.lock();
	if (!owner)return;

	//強攻撃を喰らったらガードブレイク
	if (owner->m_attackData.attackType == AttackType::heavyAttack)
	{
		owner->m_guardInfo.GurardGauge = 0.0f;//一旦ここ(毎回リセットしたくなったらExitでする
		owner->ChangeState(std::make_shared<EnemyGuardBreak>(owner));
		return;
	}


	//弱攻撃を喰らう->ゲージたまったらはじき
	owner->m_guardInfo.GurardGauge += owner->m_attackData.attackPower;

	//一旦固定値
	if (owner->m_guardInfo.GurardGauge >= owner->m_status.parryGauge)
	{
		owner->m_guardInfo.GurardGauge = 0.0f;//一旦ここ(毎回リセットしたくなったらExitでする
		owner->ChangeState(std::make_shared<EnemyParry>(owner));
		return;
	}

	owner->m_anim.ChangeAnim(owner->GetAnimName("BlockHit"), true,0.5f);

}
