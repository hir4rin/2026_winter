#include "PlayerStateFall.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"

namespace
{
	constexpr float kFallMoveSpeedMultiplier = 0.8f;//落下中の移動速度倍率(通常の0.8倍)
	constexpr int kCoyoteFrame = 6;//地面から離れてもジャンプを受け付けるフレーム数
}

PlayerStateFall::PlayerStateFall(std::weak_ptr<Player> player, bool canCoyoteJump) : PlayerState(player),
	m_canCoyoteJump(canCoyoteJump)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateFall::~PlayerStateFall()
{
}

void PlayerStateFall::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//animationの初期化
	player->m_anim.ChangeAnim(player->GetAnimName("JumpDown"), false, 1.0f);
	//下降時の初速を保存
	m_baseVel = player->m_rb.m_vel;
	m_baseVel.y = 0.0f;//y成分は移動に関係ないので、0にする
	//静止状態から落ち始める
	m_gravity = 0.0f;
}

void PlayerStateFall::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//重力の処理//だんだん落ちていくようにする//累積から毎フレーム作り直す(タイムスケールで減衰しないように)
	m_gravity += -Game::kGravity * System::GetInstance().GetTimeScale() * player->m_ownTimeScale;
	player->m_rb.m_vel.y = m_gravity;
	//地面に着いたら
	if (player->IsFloor())
	{
		//player->m_rb.m_pos.y = 0.0f;//地面に埋まらないようにする
		player->m_rb.m_vel.y = 0.0f;//y軸の速度を0にする
		//ジャンプ状態を解除
		player->m_isGround = true;//地面にいる状態にする
		//System::GetInstance().GetSoundManager().PlaySE("JumpUpAndDown");//着地音
		if (input.IsLeftStickInput())
		{
			//入力があればWalk状態に遷移する
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
			return;
		}
	}
	//地面に着いたら
	//if (player->m_rb.m_pos.y <= 0.0f)
	//{
	//	player->m_rb.m_pos.y = 0.0f;//地面に埋まらないようにする
	//	player->m_rb.m_vel.y = 0.0f;//y軸の速度を0にする
	//	//ジャンプ状態を解除
	//	player->m_isGround = true;//地面にいる状態にする
	//	if (input.IsLeftStickInput())

	//	{
	//		//入力があればWalk状態に遷移する
	//		player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
	//		return;
	//	}
	//	else
	//	{
	//		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
	//		return;
	//	}
	//}

	//コヨーテタイム//歩いて地面から離れた直後の数フレームはジャンプを受け付ける
	m_frame++;
	if (m_canCoyoteJump && m_frame <= kCoyoteFrame && input.IsTriggered("A"))
	{
		player->ChangeState(std::make_shared<PlayerStateJump>(m_owner));
		return;
	}

	//スキル攻撃
	if (input.IsTriggered("LB"))
	{
		//初めての攻撃だったら
		if (!player->m_comboInfo.isAirSkillAttack)
		{
			if (player->CanSkillAttack())
			{
				player->ChangeState(std::make_shared<PlayerStateSkillAttack>(m_owner));
				return;
			}
		}
	}

	//攻撃状態に遷移する
	if (input.IsTriggered("X"))//弱攻撃
	{
		//初めての攻撃だったら
		if (!player->m_comboInfo.isAirAttack)
		{
			player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::lightAttack));
			return;
		}
	}
	if (input.IsTriggered("Y"))//強攻撃
	{
		//初めての攻撃だったら
		if (!player->m_comboInfo.isAirAttack)
		{
			player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::heavyAttack));
			return;
		}
		
	}
	Move(input);
	//アニメーション
	player->m_anim.Update();
}

void PlayerStateFall::Exit()
{
}

void PlayerStateFall::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Fall");
#endif
}

void PlayerStateFall::Move(Input& input)
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	//Playerの移動方向を決める
	HandlerInput();
	//移動入力をとる
	Vector3 dimention = Vector3(0, 0, 0);
	if (input.IsPressed("Up"))
	{
		dimention += player->forward;
	}
	if (input.IsPressed("Down"))
	{
		dimention += player->down;
	}
	if (input.IsPressed("Right"))
	{
		dimention += player->right;
	}
	if (input.IsPressed("Left"))
	{
		dimention += player->left;
	}
	dimention = dimention.Normalize() * Game::kMoveSpeed * kFallMoveSpeedMultiplier;//移動速度は通常の0.5倍
	Vector3 velY = Vector3(0, player->m_rb.m_vel.y, 0);//y成分だけを取り出す
	player->m_rb.m_vel = m_baseVel + dimention + velY;
	//速度制限
	ClampSpeed();
	//移動している間は目標のベクトルを更新する//2次元方向のみ
	Vector3 targetVec = Vector3(player->m_rb.m_vel.x, 0.0f, player->m_rb.m_vel.z);
	if(targetVec.Magnitude() > 0.0f)player->m_targetVec = targetVec.Normalize();
	////初期化
	//Vector3 dimention = Vector3(player->m_rb.m_vel.x, 0, player->m_rb.m_vel.z);//x,z成分だけを取り出す
	//dimention = dimention.Normalize() * Game::kMoveSpeed * 0.8f;//移動速度は通常の0.8倍
	//player->m_rb.m_vel = dimention + Vector3(0, player->m_rb.m_vel.y, 0);
}

