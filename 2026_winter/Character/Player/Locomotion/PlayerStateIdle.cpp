#include "PlayerStateIdle.h"
#include "Player.h"
#include "../../../Input.h"

PlayerStateIdle::PlayerStateIdle(std::weak_ptr<Player> player) :
	PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateIdle::~PlayerStateIdle()
{
}
void PlayerStateIdle::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//animationの初期化
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle,player->GetAnimName("Idle"), true);
	//移動速度を0にする//だんだん遅くするにする予定
	//Runの後だったら専用の切り返しモーションとかやりたい
	player->m_rb.m_vel = Vector3(0, 0, 0);

}

void PlayerStateIdle::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//押し戻しの処理が続かないように消す
	player->m_rb.m_vel = Vector3(0, 0, 0);

	//鴉状態の更新
	if (input.IsPressed("LB"))
	{
		player->m_isRaven = true;
	}
	else
	{
		player->m_isRaven = false;
	}

	//移動状態に遷移する
	if (input.IsLeftStickInput())
	{
		player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
		return;
	}
	//スキル攻撃
	if (input.IsPressed("LB") && input.IsTriggered("X"))
	{
		if (player->CanSkillAttack())
		{
			player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::SkillAttack));
			return;
		}
	}
	//必殺技
	if (input.IsPressed("LB") && input.IsTriggered("Y"))
	{
		if (player->CanUltAttack())
		{
			player->ChangeState(std::make_shared<PlayerStateUlt>(m_owner));
			return;
		}
	}


	//攻撃状態に遷移する
	if (input.IsTriggered("X"))//弱攻撃
	{
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner,AttackType::lightAttack));
		return;
	}
	if (input.IsTriggered("Y"))//強攻撃
	{
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner,AttackType::heavyAttack));
		return;
	}
	
	

	//ジャンプ
	if (input.IsTriggered("A") )
	{
		player->ChangeState(std::make_shared<PlayerStateJump>(m_owner));
		return;
	}


	//アニメーションの更新
	player->m_anim.Update();
}

void PlayerStateIdle::Exit()
{
}

void PlayerStateIdle::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Idle");
}
