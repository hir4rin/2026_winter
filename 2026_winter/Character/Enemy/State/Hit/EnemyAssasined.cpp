#include "EnemyAssasined.h"
#include "../../EnemyBase.h"
#include "../../HitCol.h"
#include "Player.h"
#include "../System.h"

namespace
{
	//最初のStartのフレーム
	constexpr float kStartMaxTimer = 20.0f;
	//実行中の動く速度
	constexpr float kExecuteSpeed = 7.0f;
	//実行中のフレーム
	constexpr float kExecuteTimer = 25.0f;
}

EnemyAssasined::EnemyAssasined(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyAssasined::~EnemyAssasined()
{
}

void EnemyAssasined::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();


	//被暗殺アニメーション
	//	Execute01Victim
	//	Execute02Victim
	//	Execute03Victim
	//	FootPlantVictim
	//	RunSlashFinisher
	//	StabBehindVictim
	//	StabChestVictim
	//	fromAvobeVictim
	//	fromAvobeForwardVictim
	//	SneakStabBehindVictim


	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Execute02Victim"), false,0.7f);
	//スタートする
	m_state = AssasinState::Start;

	//キャラ同士の押し戻しを無効化
	owner->SetIsGhost(true);
	//Playerのほうを向く
	Vector3 toPlayer = player->GetRigidBody().GetPos() - owner->GetRigidBody().GetPos();
	owner->m_targetVec = toPlayer.Normalize();
}

void EnemyAssasined::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = owner->GetRigidBody().GetPos();

	Vector3 toPlayer = playerPos - enemyPos;
	toPlayer = toPlayer.Normalize();
	toPlayer.y = 0;

	switch (m_state)
	{
	case AssasinState::Start:
		m_startTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_startTimer > kStartMaxTimer)
		{
			m_state = AssasinState::Execute;
			owner->OnHeadBreak();
		}
		break;
	case AssasinState::Execute:
		//実行中
		//ちょこっとだけ動かす
		owner->m_rb.m_vel = toPlayer * -1 * kExecuteSpeed;
		m_excuteTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_excuteTimer > kExecuteTimer)
		{
			m_state = AssasinState::End;
		}

		break;
	case AssasinState::End:
		break;
	}




	//今はテスト中だから終わったらIdleに戻す
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
	}

	
}

void EnemyAssasined::Exit()
{
}

void EnemyAssasined::DebugDraw()
{
}
