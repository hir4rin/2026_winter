#include "EnemyHitAir.h"
#include "../../EnemyBase.h"

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
	//縦の初速を渡す//縦の速度は初速と重力の累積からEnemySwordman::Updateが作る
	//m_initVelYが正なら、床の上にいても重力の処理は空中扱いになる
	owner->m_initVelY = m_info.knockBackVel.y;
	owner->m_accumulatedGravity = 0.0f;
}

void EnemyHitAir::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_anim.Update(owner->m_ownTimeScale);

	//水平方向は毎フレームHitInfoの初速を与える(EnemySwordman::Updateで毎フレーム水平速度がリセットされるため)
	owner->m_rb.m_vel = Vector3(m_info.knockBackVel.x, owner->m_rb.m_vel.y, m_info.knockBackVel.z);

	//上昇しきったら(縦の速度が0以下になったら)AirStayへ
	if (owner->m_initVelY + owner->m_accumulatedGravity <= 0.0f)
	{
		owner->ChangeState(std::make_shared<EnemyAirStay>(owner));
	}
}

void EnemyHitAir::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//初速と重力の累積を戻しておく(残っていると上昇が続いたり、AirStay後に加速して落ちる)
	owner->m_initVelY = 0.0f;
	owner->m_accumulatedGravity = 0.0f;
}

void EnemyHitAir::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:HitAir");
}
