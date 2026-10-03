#include "PlayerStateWallRunKick.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"

namespace
{
	constexpr float kWallRunKickSpeed = 20.0f;//壁走りキックの速度//水平方向

	constexpr float kJumpInitVel = 10.0f;//ジャンプの初速//上方向の速度

	constexpr float kJumpForWallStayFrame = 23.0f;//壁に沿う状態に遷移するまでのフレーム数
}

PlayerStateWallRunKick::PlayerStateWallRunKick(std::weak_ptr<Player> player) : PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateWallRunKick::~PlayerStateWallRunKick()
{
}

void PlayerStateWallRunKick::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//今の速度方向(壁走りの進行方向)//水平成分だけを使う
	Vector3 velDir = player->m_rb.m_vel;
	velDir.y = 0.0f;
	if (velDir.sqMagnitude() > 0.01f)
	{
		velDir = velDir.Normalize();
	}

	//壁の法線方向//水平成分だけを使う
	Vector3 wallNormal = player->m_wallHitInfo.wallNormal;
	wallNormal.y = 0.0f;
	wallNormal = wallNormal.Normalize();

	//進行方向+法線方向で、壁から斜め前に離れる方向にする
	Vector3 DirVec = (velDir + wallNormal).Normalize();

	player->m_rb.m_vel = DirVec * kWallRunKickSpeed;//斜め前にジャンプする
	player->m_rb.m_vel += Vector3(0, kJumpInitVel, 0);//上方向にもジャンプする

	//playerの向きを速度方向にする
	player->m_targetVec = DirVec;

	//保存する
	m_InitVel = player->m_rb.m_vel;
	//重力を初期値
	m_gravity = 0.0f;

	//アニメーションの切り替え
	player->m_anim.ChangeAnim(player->GetAnimName("JumpUp"), false, 1.0f);

	//壁キックの法線を保存する//同じ壁にすぐ張り付かないように
	player->m_lastKickWallNormal = player->m_wallHitInfo.wallNormal;
}

void PlayerStateWallRunKick::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//重力の処理//だんだん落ちていくようにする//累積から毎フレーム作り直す(タイムスケールで減衰しないように)
	m_gravity += -Game::kGravity * System::GetInstance().GetTimeScale() * player->m_ownTimeScale;

	player->m_rb.m_vel = m_InitVel + Vector3(0.0f, m_gravity, 0.0f);//初速を毎フレーム与える//重力累積も与える



	//animationFrameで経過時間を取る
	if (player->m_anim.GetNowAnimFrame() >= kJumpForWallStayFrame)
	{
		//壁走りゾーンの中で壁と当たったら、また壁走りにする
		if (IsInWallZone(Collider::ColRole::WallRunZone) && CheckWall())
		{
			player->ChangeState(std::make_shared<PlayerStateWallRun>(m_owner));
			return;
		}
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

void PlayerStateWallRunKick::Exit()
{
}

void PlayerStateWallRunKick::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:WallRunKick");
#endif
}
