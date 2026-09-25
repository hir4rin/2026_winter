#include "EnemyKnockBack.h"
#include "../../EnemyBase.h"
#include "../../../System.h"

namespace
{
	constexpr float kKnockBackTime = 60.0f;//ダウンしている時間(frame)
}

EnemyKnockBack::EnemyKnockBack(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyKnockBack::~EnemyKnockBack()
{
}

void EnemyKnockBack::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	//ダウン用のアニメーションが用意できたらここで再生する
}

void EnemyKnockBack::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);
	m_downTime += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	//一定時間ダウンしたらIdleに戻る
	if (m_downTime > kKnockBackTime)
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
	}
}

void EnemyKnockBack::Exit()
{
}

void EnemyKnockBack::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:KnockBack");
}
