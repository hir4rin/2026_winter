#include "EnemyChase.h"
#include "Main/System.h"
#include "../../EnemyBase.h"
#include "Character/Player/Player.h"

EnemyChase::EnemyChase(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyChase::~EnemyChase()
{
}

void EnemyChase::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下禁止を有効化
	owner->SetPreventFall(true);
	auto player = owner->m_player.lock();
	if (!player)return;
	//animationの初期化
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("RunMain"), true,0.5f);
	owner->m_targetPos = player->GetRigidBody().GetPos();
	//Playerを見る
	owner->ToPlayerLook();

	//時間をリセット
	owner->m_chasingTime = 0.0f;

	//頭をプレイヤーの方に向ける
	owner->m_isLookAtPlayer = true;
}

void EnemyChase::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//攻撃可能な距離に入ったら、次の行動を敵に決めてもらう(初期はAttackPre)
	if (owner->SelectAttackIndex() >= 0)//選ぶのはNextOnArrived
	{
		owner->ChangeState(owner->NextOnArrived());
		return;
	}

	//定期的にプレイヤーの位置を更新する
	if (!owner->ChaseTarget(owner->m_targetPos, owner->GetMaxAttackRange(),owner->m_status.moveSpeed))
	{
		//後ほど
		//owner->m_chasingTime += 1.0f * timeScale * m_ownTimeScale;
		owner->m_chasingTime += 1.0f * System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
		//更新
		if (owner->m_chasingTime > owner->m_status.chaseTargetUpdateTime)
		{
			owner->m_chasingTime = 0.0f;
			owner->m_targetPos = owner->TargetPlayerPos();
			//Playerを見る
			owner->ToPlayerLook();
		}
	}
	//playerの位置についたら
	else
	{
		//目的地に着いたら、次の行動を敵に決めてもらう(初期は、攻撃できないのでCaution)
		owner->ChangeState(owner->NextOnArrived());
		return;
	}
}

void EnemyChase::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下禁止を無効化
	owner->SetPreventFall(false);
	//頭の向きを正面に戻す
	owner->m_isLookAtPlayer = false;
}

void EnemyChase::DebugDraw()
{
}
