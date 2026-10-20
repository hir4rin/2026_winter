#include "EnemyAirAssasined.h"
#include "../../EnemyBase.h"
#include "Collision/HitCol.h"
#include "Character/Player/Player.h"
#include "Main/System.h"

namespace
{
	//接近のフレーム(プレイヤーと合わせる)
	constexpr float kApproachFrame = 11.0f;
	//一緒に移動する速度(プレイヤーと合わせる)
	constexpr float kMoveSpeed = 24.0f;
	//一緒に移動するフレーム(プレイヤーと合わせる)
	constexpr float kMoveFrame = 14.0f;
}

EnemyAirAssasined::EnemyAirAssasined(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyAirAssasined::~EnemyAirAssasined()
{
}

void EnemyAirAssasined::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();


	//被空中暗殺アニメーション
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("fromAvobeForwardVictim"), false,1.6f);
	//スタートする
	m_state = AssasinState::Approach;

	//キャラ同士の押し戻しを無効化
	owner->SetIsGhost(true);
	//Playerのほうを向く
	Vector3 toPlayer = player->GetRigidBody().GetPos() - owner->GetRigidBody().GetPos();
	owner->m_targetVec = toPlayer.Normalize();

	//playerを見つけた判定にする
	owner->m_isPlayerFound = true;
}

void EnemyAirAssasined::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = owner->GetRigidBody().GetPos();

	Vector3 toPlayer = playerPos - enemyPos;
	toPlayer = toPlayer.Normalize();
	toPlayer.y = 0;

	m_timer += 1.0f * System::GetInstance().GetTimeScale();

	switch (m_state)
	{
	case AssasinState::Approach:
		//待つ
		if (m_timer > kApproachFrame)
		{
			m_state = AssasinState::Move;
			m_timer = 0.0f;
		}
		break;
	case AssasinState::Move:
		//プレイヤーと一緒に水平移動
		owner->m_rb.m_vel = toPlayer * -1 * kMoveSpeed;
		if (m_timer > kMoveFrame)
		{
			m_state = AssasinState::Execute;
			m_timer = 0.0f;
		}
		break;
	case AssasinState::Execute:
		//頭破壊
		owner->m_rb.m_vel = Vector3(0.0f, 0.0f, 0.0f);
		owner->OnHeadBreak();
		owner->PlayHeadBreakBlood();
		m_state = AssasinState::End;
		break;
	case AssasinState::End:
		break;
	}

	//死んだ判定
	if (m_state == AssasinState::End && owner->m_anim.GetAnimEndFlag())
	{
		owner->FinisherPerformanceProcess();
	}
}

void EnemyAirAssasined::Exit()
{
}

void EnemyAirAssasined::DebugDraw()
{
}
