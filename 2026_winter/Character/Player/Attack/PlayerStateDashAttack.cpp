#include "PlayerStateDashAttack.h"
#include "Player.h"
#include "../../../Input.h"


PlayerStateDashAttack::PlayerStateDashAttack(std::weak_ptr<Player> player):
	PlayerState(player)	
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateDashAttack::~PlayerStateDashAttack()
{
}

void PlayerStateDashAttack::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//animationの初期化
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle,player->GetAnimName("DashAttack"), false);
}

void PlayerStateDashAttack::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();
	AttackMove();
	//攻撃が終了したら、Idle状態に遷移する
	if (player->m_anim.GetAnimEndFlag())
	{
		if (input.IsLeftStickInput())
		{
			//入力があればMove状態に遷移する
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
			return;
		}
	}
	//アニメーションの更新
	player->m_anim.Update();

}

void PlayerStateDashAttack::Exit()
{
}

void PlayerStateDashAttack::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:DashAttack");
}

void PlayerStateDashAttack::AttackMove()
{
	auto player = m_owner.lock();
	if (!player) return;
	const ComboNode& node = player->m_comboChain[ComboIndex::DashAttack];
	float rate = player->m_anim.GetAnimRate();//アニメーションの進行率を取得

	//コンボノードで設定された時間内だけ突進
	if (rate < node.moveFrame)
	{
		player->m_rb.m_vel = player->m_targetVec * node.moveSpeedX;//攻撃の最初の数秒は前に突進する
	}
	else
	{
		player->m_rb.m_vel = Vector3(0, 0, 0);//突進が終わったら、速度を0にする
	}
}
