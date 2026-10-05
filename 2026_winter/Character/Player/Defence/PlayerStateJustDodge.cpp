#include "PlayerStateJustDodge.h"
#include "Player.h"
#include "../../../Input.h"
#include "../Game.h"
#include "../System.h"

namespace
{
	constexpr float kAnimSpeed = 0.8f;//アニメーションのブレンド率
	constexpr float kAnimEndFrame = 41.0f;
	
	constexpr float kJustDodgeMoveSpeed = 10.0f;
}

PlayerStateJustDodge::PlayerStateJustDodge(std::weak_ptr<Player> player, PlayerStateDodge::AvoidState avoidState)
	:PlayerState(player), m_avoidState(avoidState)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateJustDodge::~PlayerStateJustDodge()
{
}

void PlayerStateJustDodge::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//ジャスト回避用のアニメーションにする//前回避か後ろ回避かで変える
	if (m_avoidState == PlayerStateDodge::AvoidState::Backward)
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JustDodgeBackward"), false, kAnimSpeed, kAnimEndFrame);
	}
	else
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JustDodgeForward"), false, kAnimSpeed, kAnimEndFrame);
	}
}

void PlayerStateJustDodge::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	float timeScale = System::GetInstance().GetTimeScale();

	//PlayerStateDodgeと同じ移動量で、前回避なら前、後ろ回避なら後ろに動かす
	if (m_avoidState == PlayerStateDodge::AvoidState::Backward)
	{
		player->m_rb.m_vel = player->m_targetVec.Normalize() * kJustDodgeMoveSpeed * -1;
	}
	else
	{
		player->m_rb.m_vel = player->m_targetVec.Normalize() * kJustDodgeMoveSpeed;
	}

	//ジャスト回避後の確定反撃も後ほど用意する


	//モーションが終わったら、入力の有無でMoveかIdleに遷移する
	if (player->m_anim.GetAnimEndFlag())
	{
		if (input.IsLeftStickInput())
		{
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
			return;
		}
	}

	//自分のタイムスケールを渡して、世界がスローでもプレイヤーのアニメーションは通常速度で再生する
	player->m_anim.Update(player->m_ownTimeScale);
}

void PlayerStateJustDodge::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;

	//プレイヤーのタイムスケールを調整する
	player->SetOwnTimeScale(1.0f);
	System::GetInstance().SetTimeScale(1.0f);
}

void PlayerStateJustDodge::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(0, 16, GetColor(255, 0, 255), "PlayerState:JustDodge");
#endif
}
