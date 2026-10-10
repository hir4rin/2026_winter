#include "EnemyGuard.h"
#include "../../../System.h"
#include "../../EnemyBase.h"



namespace
{
	//一旦
	constexpr float kGuardMaxTimer = 120.0f;



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

	if (timer >= kGuardMaxTimer)
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(m_owner));
		return;
	}

	//ガード喰らい状態が終わったら、またアニメーションを元に戻す
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->m_anim.ChangeAnim(owner->GetAnimName("BlockLoop"), true,0.5f); 
	}


	owner->m_anim.Update(owner->m_ownTimeScale);
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
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:Guard");
}

void EnemyGuard::OnGuardHit()
{
	//アニメーションを変更する処理//これが終わった後また戻さないといけない
	auto owner = m_owner.lock();
	if (!owner)return;


	owner->m_guardInfo.GurardGauge += owner->m_attackData.attackPower;

	//一旦固定値
	if (owner->m_guardInfo.GurardGauge >= 100)
	{
		owner->m_guardInfo.GurardGauge = 0.0f;//一旦ここ(毎回リセットしたくなったらExitでする
		owner->ChangeState(std::make_shared<EnemyGuardBreak>(owner));
		return;
	}

	owner->m_anim.ChangeAnim(owner->GetAnimName("BlockHit"), true);

}
