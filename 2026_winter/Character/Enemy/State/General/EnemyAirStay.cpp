#include "EnemyAirStay.h"
#include "../../EnemyBase.h"
#include "../../../System.h"

namespace
{
	constexpr float kAirStayTime = 60.0f;//空中で浮いている時間(frame)
}

EnemyAirStay::EnemyAirStay(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyAirStay::~EnemyAirStay()
{
}

void EnemyAirStay::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_rb.m_vel = Vector3(0, 0, 0);
}

void EnemyAirStay::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);
	m_airTime += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;

	//浮遊させる//重力はかけない
	owner->m_rb.m_vel = Vector3(0, 0, 0);

	//一定時間浮いたら落下に移行
	if (m_airTime > kAirStayTime)
	{
		owner->ChangeState(std::make_shared<EnemyAirFall>(owner));
	}
}

void EnemyAirStay::Exit()
{
}

void EnemyAirStay::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:AirStay");
}
