#include "PlayerStateResultMove.h"
#include "Player.h"

namespace
{
	constexpr float kResultMoveAnimSpeed = 0.4f;//リザルト演出時のアニメーション再生速度
	constexpr float kResultMoveStopRate = 0.6f;//アニメーションを止める進行率
}

PlayerStateResultMove::PlayerStateResultMove(std::weak_ptr<Player> player):
	PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateResultMove::~PlayerStateResultMove()
{
}

void PlayerStateResultMove::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//アニメーションを流す
	player->m_anim.ChangeAnim(player->GetAnimName("Walking"), true, kResultMoveAnimSpeed);

}

void PlayerStateResultMove::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	//アニメーションが5割を超えたらアニメーションを止める
	//if (player->m_anim.GetAnimRate() > kResultMoveStopRate)
	//{
	//	//player->m_anim.
	//}
	//else
	//{
	//	
	//}
	player->m_anim.Update();
}

void PlayerStateResultMove::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;
}

void PlayerStateResultMove::DebugDraw()
{
}
