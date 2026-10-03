#include "PlayerStateWallKick.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"

namespace
{
	constexpr float kWallKickSpeed = 10.0f;//壁キックの速度

	constexpr float KkJumpInitVel = 20.0f;//ジャンプの初速//上方向の速度
}

PlayerStateWallKick::PlayerStateWallKick(std::weak_ptr<Player> player) : PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateWallKick::~PlayerStateWallKick()
{
}

void PlayerStateWallKick::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	
	//壁の法線方向にジャンプする//斜めのほうがいいかも//最期ようにどうせ重力は必要だった
	Vector3 DirVec = player->m_wallHitInfo.wallNormal;
	DirVec.y = 0.0f;//壁の法線方向の水平成分だけを使う

	player->m_rb.m_vel = DirVec.Normalize() * kWallKickSpeed;//壁の法線方向にジャンプする
	player->m_rb.m_vel += Vector3(0, KkJumpInitVel, 0);//上方向にもジャンプする

	//playerの向きを速度方向にする
	player->m_targetVec = DirVec.Normalize();

	//保存する
	m_InitVel = player->m_rb.m_vel;
	//重力を初期値
	m_gravity = 0.0f;

	//アニメーションの切り替え
	player->m_anim.ChangeAnim(player->GetAnimName("JumpUp"), false, 1.0f, -1.0f);

	//壁キックの法線を保存する
	player->m_lastKickWallNormal = player->m_wallHitInfo.wallNormal;
}

void PlayerStateWallKick::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//重力の処理//だんだん落ちていくようにする//累積から毎フレーム作り直す(タイムスケールで減衰しないように)
	m_gravity += -Game::kGravity * System::GetInstance().GetTimeScale() * player->m_ownTimeScale;

	player->m_rb.m_vel = m_InitVel + Vector3(0.0f,m_gravity,0.0f);//初速を毎フレーム与える//重力累積も与える

	//壁キックゾーンの中で壁と当たったらまたStayにする
	if(IsInWallZone(Collider::ColRole::WallKickZone) && CheckWall())
	{


		player->ChangeState(std::make_shared<PlayerStateWallStay>(m_owner));
		return;
	}

	//速度が0以下になったらFallにする
	if (player->m_rb.m_vel.y <= 0.0f)
	{
		//この時、最期にけった壁の法線をクリアする
		player->m_lastKickWallNormal = Vector3(0, 0, 0);

		player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));
		return;
	}





	//アニメーション
	player->m_anim.Update();
}

void PlayerStateWallKick::Exit()
{
}

void PlayerStateWallKick::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:WallKick");
#endif
}
