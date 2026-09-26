#include "EnemyHitAir.h"
#include "../../EnemyBase.h"
#include "../../../System.h"
#include "../Game.h"

EnemyHitAir::EnemyHitAir(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info) :
	EnemyStateBase(owner),
	m_info(info)
{
}

EnemyHitAir::~EnemyHitAir()
{
}

void EnemyHitAir::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Hit"), false);
	//縦の速度は初速(m_info.knockBackVel.y)と重力の累積(m_gravity)から作る
	m_gravity = 0.0f;
}

void EnemyHitAir::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);

	//重力
	//m_velはCollisionManager::AddVelocityでタイムスケールを掛けて上書きされるので、初速と累積から毎フレーム作り直す
	//重力の累積はステートで持つ(ステートごとに0から始める)
	m_gravity += -Game::kGravity * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	owner->m_rb.m_vel = m_info.knockBackVel + Vector3(0, m_gravity, 0);

	//上昇しきったら(縦の速度が0以下になったら)AirStayへ
	if (m_info.knockBackVel.y + m_gravity <= 0.0f)
	{
		owner->ChangeState(std::make_shared<EnemyAirStay>(owner));
	}
}

void EnemyHitAir::Exit()
{
}

void EnemyHitAir::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:HitAir");
}
