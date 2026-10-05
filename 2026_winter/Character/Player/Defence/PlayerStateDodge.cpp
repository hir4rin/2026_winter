#include "PlayerStateDodge.h"
#include "Player.h"
#include "../../../Input.h"
#include "../Game.h"

PlayerStateDodge::PlayerStateDodge(std::weak_ptr<Player> player):PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateDodge::~PlayerStateDodge()
{
}

void PlayerStateDodge::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();
	//カメラから見たPlayerの正面方向を取る
	HandlerInput();
	//playerの向きを更新
	 int hasInput = 0;

	if (input.IsPressed("Up"))
	{
		player->m_targetVec = player->forward;
		hasInput++;
	}
	if (input.IsPressed("Down"))
	{
		player->m_targetVec = player->down;
		hasInput++;
	}
	if (input.IsPressed("Right"))
	{
		player->m_targetVec = player->right;
		hasInput++;
	}
	if (input.IsPressed("Left"))
	{
		player->m_targetVec = player->left;
		hasInput++;
	}


	//animationの初期化//ロックオン中もまた分ける
	if (hasInput == 0)
	{
		//入力がなかった場合
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("DodgeBackward"), true, 0.8f);
		m_avoidState = AvoidState::Backward;
	}
	else
	{
		//入力があった場合
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("DodgeForward"), true, 0.8f);
		m_avoidState = AvoidState::Forward;

	}

}

void PlayerStateDodge::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	if (m_avoidState == AvoidState::Backward)
	{
		player->m_rb.m_vel = player->m_targetVec.Normalize() * Game::kDodgeSpeed * -1;
	}
	else
	{
		player->m_rb.m_vel = player->m_targetVec.Normalize() * Game::kDodgeSpeed;
	}

	if (player->m_anim.GetAnimRate() > 0.3f)
	{
		if (input.IsLeftStickInput())
		{
			//入力があればWalk状態に遷移する
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
			return;
		}
	}

	player->m_anim.Update();
}

void PlayerStateDodge::Exit()
{
}

void PlayerStateDodge::DebugDraw()
{
}
