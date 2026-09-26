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

	//浮遊させる//EnemySwordman::Updateの重力が後からかかるので、少し沈む場合は重力を止める仕組みが必要
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	//重力が累積しないようにする(累積させると浮遊中に加速して落ちる)
	owner->m_accumulatedGravity = 0.0f;

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
