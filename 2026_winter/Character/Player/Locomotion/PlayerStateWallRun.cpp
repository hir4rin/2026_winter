#include "PlayerStateWallRun.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"

namespace
{
}

PlayerStateWallRun::PlayerStateWallRun(std::weak_ptr<Player> player) : PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateWallRun::~PlayerStateWallRun()
{
}

void PlayerStateWallRun::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
}

void PlayerStateWallRun::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//アニメーション
	player->m_anim.Update();
}

void PlayerStateWallRun::Exit()
{
}

void PlayerStateWallRun::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:WallRun");
#endif
}
