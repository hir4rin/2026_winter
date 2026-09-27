#include "PlayerStateAssasin.h"
#include "Player.h"
#include "../../Enemy/EnemyBase.h"

PlayerStateAssasin::PlayerStateAssasin(std::weak_ptr<Player> player) :PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateAssasin::~PlayerStateAssasin()
{
}

void PlayerStateAssasin::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//暗殺名前一覧
	//	Execute01->Execute01Victim     移動させないとださい　△
	//	Execute02->Execute02Victim　　まあまあ　　　採用
	//	Execute03->Execute03Victim   一瞬
	//	Execute04->FootPlantVictim   一瞬そこそこ　　絶技採用
	//	Assasin01->RunSlashFinisher　ちょっと地味　移動ぎり感すごい
	//	Assasin02->fromAvobeForwardVictim 地上からなら微妙
	//	Assasin03->StabBehindVictim　まあまあ　でも終わりポーズがださい
	//	Assasin04->StabChestVictim   わりかし


	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Execute02"), false, 0.7f);

	//敵のステートを変える
	auto assasinTarget = player->GetAssasinTarget();
	if (!assasinTarget)return;
	assasinTarget->OnAssasined();


}

void PlayerStateAssasin::Update()
{
	auto player = m_owner.lock();
	if (!player) return;


	if (player->m_anim.GetAnimEndFlag())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
		return;
	}
	player->m_anim.Update();
}

void PlayerStateAssasin::Exit()
{
}

void PlayerStateAssasin::DebugDraw()
{
}
