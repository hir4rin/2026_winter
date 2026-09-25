#include "EnemyHitGround.h"
#include "../../EnemyBase.h"
#include "../../../System.h"

EnemyHitGround::EnemyHitGround(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info) :
	EnemyStateBase(owner),
	m_info(info)
{
}

EnemyHitGround::~EnemyHitGround()
{
}

void EnemyHitGround::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Hit"), false);
}

void EnemyHitGround::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);
	m_frame += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	//水平方向の初速を時間とともに減衰させる//縦はEnemySwordman::Updateの重力に任せる
	float rate = 0.0f;
	if (m_info.duration > 0.0f)
	{
		rate = 1.0f - (m_frame / m_info.duration);
		if (rate < 0.0f)rate = 0.0f;
	}
	owner->m_rb.m_vel = Vector3(m_info.knockBackVel.x * rate,
		owner->m_rb.m_vel.y,
		m_info.knockBackVel.z * rate);

	//吹き飛ばしが終わったら、床にいればIdle、空中ならAirStayへ
	if (m_frame >= m_info.duration)
	{
		if (owner->IsFloor())
		{
			owner->ChangeState(std::make_shared<EnemyIdle>(owner));
		}
		else
		{
			owner->ChangeState(std::make_shared<EnemyAirStay>(owner));
		}
	}
}

void EnemyHitGround::Exit()
{
}

void EnemyHitGround::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:HitGround");
}
