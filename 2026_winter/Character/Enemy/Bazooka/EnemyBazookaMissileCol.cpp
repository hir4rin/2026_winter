#include "EnemyBazookaMissileCol.h"
#include "Stage/Stage.h"

EnemyBazookaMissileCol::EnemyBazookaMissileCol()
{
}

EnemyBazookaMissileCol::~EnemyBazookaMissileCol()
{
}

void EnemyBazookaMissileCol::OnCollision(Collider& other)
{
	//プレイヤー(ジャスト回避の判定は除く)か置いてある物に触れたら爆発
	if(other.GetFaction() == Faction::Player && other.GetRole() != ColRole::JustDodge)
	{
		m_isTouched = true;
		return;
	}

	if(other.GetFaction() == Faction::StaticObject)
	{
		m_isTouched = true;
		return;
	}

}

void EnemyBazookaMissileCol::ApplyPos()
{
	//位置はSetPosで入れるので何もしない
}

void EnemyBazookaMissileCol::SetPos(const Vector3& pos)
{
	m_rb.m_pos = pos;
	m_rb.m_vel = Vector3(0, 0, 0);
}

int EnemyBazookaMissileCol::GetStageHandle() const
{
	auto stage = m_stage.lock();
	if (!stage) return -1;
	return stage->GetStageModelHandle();
}
