#include "EnemyHitDrop.h"
#include "../../EnemyBase.h"
#include "../../../System.h"
#include "../Game.h"

EnemyHitDrop::EnemyHitDrop(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info) :
	EnemyStateBase(owner),
	m_info(info)
{
}

EnemyHitDrop::~EnemyHitDrop()
{
}

void EnemyHitDrop::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Hit"), false);
	//縦の速度は初速(m_info.knockBackVel.y)と重力の累積(m_gravity)から作る
	m_gravity = 0.0f;
}

void EnemyHitDrop::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);

	//重力
	//m_velはCollisionManager::AddVelocityでタイムスケールを掛けて上書きされるので、初速と累積から毎フレーム作り直す
	//重力の累積はステートで持つ(ステートごとに0から始める)
	m_gravity += -Game::kGravity * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	owner->m_rb.m_vel = m_info.knockBackVel + Vector3(0, m_gravity, 0);

	//落下中に床に着いたら着地//打ち上げ直後はまだ床の上にいるので判定しない
	bool isFalling = m_info.knockBackVel.y + m_gravity < 0.0f;
	if (owner->IsFloor() && isFalling)
	{
		owner->m_rb.m_vel = Vector3(0, 0, 0);
		//死亡予定ならDie
		if (owner->m_isDieOut)
		{
			owner->m_isDead = true;
			owner->ChangeState(std::make_shared<EnemyDie>(owner));
			return;
		}

		//地面についたらknockDown状態に入る//当たり判定がでて吹き飛ぶので、そちらで対応
		owner->ChangeState(std::make_shared<EnemyKnockDown>(owner));
	}
}

void EnemyHitDrop::Exit()
{
}

void EnemyHitDrop::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:HitDrop");
}
