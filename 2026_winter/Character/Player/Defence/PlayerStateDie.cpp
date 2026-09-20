#include "PlayerStateDie.h"
#include "Player.h"

PlayerStateDie::PlayerStateDie(std::weak_ptr<Player> player, CharacterBase::AttackData& data):
	PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする//trueで破棄されている
	if (m_owner.expired())return;
	auto pPlayer = m_owner.lock();
	if (!pPlayer) return;

	//被ダメのattackDataを更新する
	pPlayer->m_attackData = data;
}

PlayerStateDie::~PlayerStateDie()
{
}

void PlayerStateDie::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;


	//アニメーションを流す
	player->m_anim.ChangeAnim(player->GetAnimName("Death"), false, 0.6f);
	//やられ判定を消す
	//CollisionManager::GetInstance().ReleaseCollider(player->GetHitCol());

}

void PlayerStateDie::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	//死亡アニメーションが終わったら、ゲームオーバーシーンに移行する
	if(player->m_anim.GetAnimEndFlag())
	{
		player->m_isDead = true;
		return;
	}



	player->m_anim.Update();

}

void PlayerStateDie::Exit()
{
}

void PlayerStateDie::DebugDraw()
{
}
