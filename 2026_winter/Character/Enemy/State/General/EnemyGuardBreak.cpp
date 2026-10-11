#include "EnemyGuardBreak.h"
#include "../../../System.h"
#include "../../EnemyBase.h"



namespace
{
	//一旦//Middle(ダウン中)の時間
	constexpr float kMiddleMaxTimer = 60.0f;
}

EnemyGuardBreak::EnemyGuardBreak(std::weak_ptr<EnemyBase> owner):
	EnemyStateBase(owner)
{
}

EnemyGuardBreak::~EnemyGuardBreak()
{
}

void EnemyGuardBreak::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;


	owner->m_anim.ChangeAnim(owner->GetAnimName("KnockDownStart"),false,0.5f,26.0f);
	m_state = GuardBreakState::Start;
}

void EnemyGuardBreak::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	switch (m_state)
	{
	case GuardBreakState::Start:
		//倒れ始めが終わったらダウン中へ
		if (owner->m_anim.GetAnimEndFlag())
		{
			m_state = GuardBreakState::Middle;
			owner->m_anim.ChangeAnim(owner->GetAnimName("KnockDownLoop"), true,0.5f);
		}
		break;
	case GuardBreakState::Middle:
		m_middleTimer += 1.0f * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
		//一定時間ダウンしたら起き上がりへ
		if (m_middleTimer >= kMiddleMaxTimer)
		{
			m_state = GuardBreakState::End;
			owner->m_anim.ChangeAnim(owner->GetAnimName("KnockDownEnd"), false,0.5f);
		}
		break;
	case GuardBreakState::End:
		//起き上がりが終わったら立ち直った後の行動へ(初期はIdle)
		if (owner->m_anim.GetAnimEndFlag())
		{
			owner->ChangeState(owner->NextAfterRecover());
			return;
		}
		break;
	}

}

void EnemyGuardBreak::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;

}

void EnemyGuardBreak::DebugDraw()
{
}
