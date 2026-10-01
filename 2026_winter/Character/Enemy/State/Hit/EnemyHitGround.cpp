#include "EnemyHitGround.h"
#include "../../EnemyBase.h"
#include "../../../System.h"
#include "Player.h"

namespace
{
	constexpr float kEnemyDistance = 50.0f;
	constexpr float kToTargetPower = 3.0f;//プレイヤーの正面に行くようにknockBackする力

}

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

	auto player = owner->m_player.lock();
	if (player)
	{
		//Enemy->Playerのベクトルに吹き飛ばす力を加える//プレイヤーの正面に行くようにknockBackする//
		Vector3 front = player->GetTargetVec();
		Vector3 pos = player->GetNextPos();
		Vector3 TargetPos = pos + front * kEnemyDistance;
		Vector3 toTarget = (TargetPos - owner->m_rb.m_pos).Normalize() * kToTargetPower;
		toTarget.y = 0.0f;

		Vector3 knockBackDir = (owner->m_rb.m_pos - player->GetRigidBody().GetPos()).Normalize();
		knockBackDir.y = 0.0f;//y軸の吹き飛ばしはなし
		//吹き飛ばしの強さは吹き飛ばし方向にだけ掛ける(吸着の向きが負の力で反転しないように)
		owner->m_rb.m_vel += (knockBackDir * m_info.knockBackPowerXZ + toTarget) * rate;
	}


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
