#include "EnemyPartBrokenKilled.h"
#include "../../EnemyBase.h"
#include "../../HitCol.h"
#include "Player.h"
#include "../System.h"

namespace
{
	//最初のStartのフレーム
	const float kStartMaxTimer = 23.07 * 1.0f;//playerの再生速度分遅くする
	//頭が取れるタイミング
	const float kHeadBrokenTime = 37.0f * 1.0f;//playerのアニメーションの再生速度分遅くする

	//実行中の動く速度
	constexpr float kExecuteSpeed = 7.0f;
	//実行中のフレーム
	constexpr float kExecuteTimer = 25.0f;

	//パターンBでのステート間のフレーム
	constexpr float kStartMoveFrame = 21.68f;
	constexpr float kEndMoveFrame = 39.0f;
}

EnemyPartBrokenKilled::EnemyPartBrokenKilled(std::weak_ptr<EnemyBase> owner, CharacterBase::PartBrokenPattern pattern):EnemyStateBase(owner),
	m_pattern(pattern)
{
}

EnemyPartBrokenKilled::~EnemyPartBrokenKilled()
{
}

void EnemyPartBrokenKilled::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();

	//キャラ同士の押し戻しを無効化
	owner->SetIsGhost(true);

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

	switch (m_pattern)
	{
	case CharacterBase::PartBrokenPattern::A:
	{
		//最初はヒットアニメーションを流し、そのあとにアニメーションを流す
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Hit"), false, 1.0f);

		//owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("StabChestVictim"), false, 1.0f);
		//スタートする
		m_state = PartBrokenKill::Start;

		//Playerのほうを向く
		Vector3 toPlayer = player->GetRigidBody().GetPos() - owner->GetRigidBody().GetPos();
		owner->m_targetVec = toPlayer.Normalize() * -1;
		//owner->SetRotY(atan2f(owner->m_targetVec.x, owner->m_targetVec.z) - DX_PI_F);
	}
		break;
	case CharacterBase::PartBrokenPattern::B:
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("RunSlashFinisher"), false, 1.0f);
		//スタートする
		m_state = PartBrokenKill::Start;
	
		//Playerのほうを向く
		Vector3 toPlayer = player->GetRigidBody().GetPos() - owner->GetRigidBody().GetPos();
		owner->m_targetVec = toPlayer.Normalize();
		//owner->SetRotY(atan2f(owner->m_targetVec.x, owner->m_targetVec.z) - DX_PI_F);
		break;
	}
}

void EnemyPartBrokenKilled::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();

	switch (m_pattern)
	{
	case CharacterBase::PartBrokenPattern::A:
		PatternAUpdate();
		break;
	case CharacterBase::PartBrokenPattern::B:
		PatternBUpdate();
		break;
	}
	

}

void EnemyPartBrokenKilled::Exit()
{
}

void EnemyPartBrokenKilled::DebugDraw()
{
}

void EnemyPartBrokenKilled::PatternAUpdate()
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
	case PartBrokenKill::Start:
		m_startTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_startTimer > kStartMaxTimer)
		{
			m_state = PartBrokenKill::Execute;
			//処刑アニメーション再生
			owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("StabChestVictim"), false, 1.0f);
		}
		break;
	case PartBrokenKill::Execute:
		//実行中
		//ちょこっとだけ動かす
		//owner->m_rb.m_vel = toPlayer * -1 * kExecuteSpeed;
		m_excuteTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_excuteTimer > kHeadBrokenTime)
		{
			owner->OnHeadBreak();
			m_state = PartBrokenKill::End;
		}
		break;
	case PartBrokenKill::End:
		break;
	}

	//今はテスト中だから終わったらIdleに戻す
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
	}
}

void EnemyPartBrokenKilled::PatternBUpdate()
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
	case PartBrokenKill::Start:
		m_startTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_startTimer > kStartMoveFrame)
		{
			m_state = PartBrokenKill::Execute;
		}
		break;
	case PartBrokenKill::Execute:
		//実行中
		//ちょこっとだけ動かす
	//	owner->m_rb.m_vel = toPlayer * -1 * kExecuteSpeed;
		m_excuteTimer += 1.0f * System::GetInstance().GetTimeScale();

		//
		if (m_excuteTimer > (kEndMoveFrame - kStartMoveFrame))//()は実行中のフレーム間だけを確保した
		{
			owner->OnHeadBreak();
			m_state = PartBrokenKill::End;
		}
		break;
	case PartBrokenKill::End:
		break;
	}

	//今はテスト中だから終わったらIdleに戻す
	if (owner->m_anim.GetAnimEndFlag())
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
	}
}