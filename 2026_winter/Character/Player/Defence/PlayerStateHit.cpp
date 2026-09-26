#include "PlayerStateHit.h"
#include "Player.h"
#include "../../../Input.h"

PlayerStateHit::PlayerStateHit(std::weak_ptr<Player> player, const CharacterBase::AttackData& data)
	:PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする//trueで破棄されている
	if (m_owner.expired())return;
}

PlayerStateHit::~PlayerStateHit()
{
}

void PlayerStateHit::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//animationの初期化
	player->m_anim.ChangeAnim(player->GetAnimName("Hit"), true);
}

void PlayerStateHit::Update()
{
	//モーションが終わったらIdleに遷移する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();


	if(player->m_anim.GetAnimEndFlag())
	{
		if (player->IsFloor())
		{
			//入力の有無で状態を変える
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
		//空中にいるとき
		else
		{
			player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));
			return;
		}
		
	}
	player->m_anim.Update();

}

void PlayerStateHit::Exit()
{
	//hitの種類によってはここで無敵時間をしてもよい
	auto player = m_owner.lock();
	if (!player) return;

	player->m_damageInfo.damageTimer = player->m_damageInfo.kDamageTime;//無敵時間を設定する

}

void PlayerStateHit::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Hit");
#endif
}
