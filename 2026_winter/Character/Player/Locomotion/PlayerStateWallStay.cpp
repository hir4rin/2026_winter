#include "PlayerStateWallStay.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"

namespace
{
}

PlayerStateWallStay::PlayerStateWallStay(std::weak_ptr<Player> player) : PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateWallStay::~PlayerStateWallStay()
{
}

void PlayerStateWallStay::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//壁の位置にプレイヤーの足元を合わせる

	player->m_rb.m_vel = Vector3(0, 0, 0);

	//壁の法線方向にプレイヤーの向きを変える
	Vector3 DirVec = player->m_wallHitInfo.wallNormal;
	DirVec.y = 0.0f;
	player->m_targetVec = DirVec.Normalize();

	//アニメーションの切り替え
	player->m_anim.ChangeAnim(player->GetAnimName("JumpDown"), false, 2.0f);
}

void PlayerStateWallStay::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	if (input.IsTriggered("A"))
	{
		player->ChangeState(std::make_shared<PlayerStateWallKick>(m_owner));
		return;
	}

	//アニメーション
	player->m_anim.Update();
}

void PlayerStateWallStay::Exit()
{
}

void PlayerStateWallStay::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:WallStay");
#endif
}
