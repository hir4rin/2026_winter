#include "PlayerStateGuardBreakChance.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"
#include "Character/Enemy/State/Defence/EnemyGuardBreak.h"
#include "Main/Input.h"
#include "Main/System.h"

namespace
{
	constexpr float kChanceTime = 60.0f;//チャンスの長さ(F)//仮
}

PlayerStateGuardBreakChance::PlayerStateGuardBreakChance(std::weak_ptr<Player> player) :
	PlayerState(player)
{
	auto owner = m_owner.lock();
	//playerが既に破棄されていたら早期リターンする
	if (!owner)return;
	//ブレイクした敵をプレイヤーから受け取る
	m_target = owner->m_guardBreakChanceTarget;
}

PlayerStateGuardBreakChance::~PlayerStateGuardBreakChance()
{
}

void PlayerStateGuardBreakChance::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//仮のアニメ
	//player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Idle"), true);
	m_timer = 0.0f;

	//TODO: 専用カメラ・スロー・演出の開始
	System::GetInstance().SetTimeScale(0.2f);
}

void PlayerStateGuardBreakChance::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//その場で止まる
	player->m_rb.m_vel = Vector3(0, 0, 0);

	//敵が消えた・ブレイクが終わったら終了
	auto target = m_target.lock();
	if (!target || !std::dynamic_pointer_cast<EnemyGuardBreak>(target->GetCurrentState()))
	{
		ToLocomotion();
		return;
	}

	//打ち上げ派生
	if (input.IsTriggered("Y"))
	{
		player->ChangeState(std::make_shared<PlayerStateGuardBreakAttack>(m_owner));
		return;
	}
	//回避
	if (input.IsTriggered("B"))
	{
		player->ChangeState(std::make_shared<PlayerStateDodge>(m_owner));
		return;
	}

	//時間切れ
	m_timer += 1.0f * System::GetInstance().GetTimeScale();
	if (m_timer >= kChanceTime)
	{
		ToLocomotion();
		return;
	}

	player->m_anim.Update();
}

void PlayerStateGuardBreakChance::Exit()
{
	//TODO: 専用カメラ・スロー・演出の終了
	System::GetInstance().SetTimeScale(1.0f);
}

void PlayerStateGuardBreakChance::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:GuardBreakChance %.0f/%.0f", m_timer, kChanceTime);
}

void PlayerStateGuardBreakChance::ToLocomotion()
{
	auto player = m_owner.lock();
	if (!player) return;
	if (Input::GetInstance().IsLeftStickInput())
	{
		player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
	}
	else
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
	}
}
