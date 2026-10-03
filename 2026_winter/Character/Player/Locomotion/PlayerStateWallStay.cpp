#include "PlayerStateWallStay.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"

namespace
{
	constexpr float kWallStayGap = 0.1f;//壁とカプセルの間のすき間//食い込み防止
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

	//壁の法線(XZ平面)
	Vector3 wallNormalXZ = player->m_wallHitInfo.wallNormal;
	wallNormalXZ.y = 0.0f;
	wallNormalXZ = wallNormalXZ.Normalize();

	//壁の位置にプレイヤーの足元を合わせる//高さはそのまま
	//壁ぴったりだとカプセルが食い込むので、法線方向に半径+すき間だけ離す
	Vector3 fitPos = player->m_wallHitInfo.hitPos + wallNormalXZ * (player->GetRadius() + kWallStayGap);
	player->m_rb.m_pos.x = fitPos.x;
	player->m_rb.m_pos.z = fitPos.z;

	player->m_rb.m_vel = Vector3(0, 0, 0);

	//壁の法線方向にプレイヤーの向きを変える
	player->m_targetVec = wallNormalXZ;

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
