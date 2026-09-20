#include "PlayerStateJump.h"
#include "PlayerStateJump.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"


namespace
{

	constexpr float kJumpInitVel = 20.0f;//ジャンプの初速//この数値を変えることで、ジャンプの高さを調整できる
	constexpr float kJumpMoveSpeedMultiplier = 0.5f;//ジャンプ中の移動速度倍率(通常の0.5倍)
}


PlayerStateJump::PlayerStateJump(std::weak_ptr<Player> player) : PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateJump::~PlayerStateJump()
{
}

void PlayerStateJump::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//animationの初期化
	
	//上昇速度を与える
	player->m_rb.m_vel.y = kJumpInitVel;//ジャンプの初速//この数値を変えることで、ジャンプの高さを調整できる
	//ジャンプ状態
	player->m_isGround = false;//地面にいない状態にする
	player->SetIsFloor(false);//地面にいない状態にする
	//ジャンプ開始時の移動速度を保存する
	m_baseVel = player->m_rb.m_vel;
	m_baseVel.y = 0.0f;//y成分は移動に関係ないので、0にする
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JumpUp"), false, 1.0f);
	//System::GetInstance().GetSoundManager().PlaySE("JumpUpAndDown");
}

void PlayerStateJump::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	float timeScale = System::GetInstance().GetTimeScale();
	player->m_rb.m_vel += Vector3(0, -Game::kGravity, 0) * timeScale;//重力の処理

	//スキル攻撃
	if (input.IsPressed("LB") && input.IsTriggered("X"))
	{
		//初めての攻撃だったら
		if (!player->m_comboInfo.isAirSkillAttack)
		{
			if (player->CanSkillAttack())
			{
				player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::SkillAttack));
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

	if (player->m_rb.m_vel.y <= 0.0f)//下降中に移行
	{
		player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));//Fall状態に遷移する
		return;
	}
	//移動処理
	Move(input);

	//アニメーション
	player->m_anim.Update();
}

void PlayerStateJump::Exit()
{
}

void PlayerStateJump::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Jump");
#endif
}

void PlayerStateJump::Move(Input& input)
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
	dimention = dimention.Normalize() * Game::kMoveSpeed * kJumpMoveSpeedMultiplier;//移動速度は通常の0.5倍
	Vector3 velY = Vector3(0, player->m_rb.m_vel.y, 0);//y成分だけを取り出す
	player->m_rb.m_vel = m_baseVel + dimention + velY;
	//速度制限
	ClampSpeed();

	//移動している間は目標のベクトルを更新する//2次元方向のみ
	Vector3 targetVec = Vector3(player->m_rb.m_vel.x, 0.0f, player->m_rb.m_vel.z);
	if (targetVec.Magnitude() > 0.0f)player->m_targetVec = targetVec.Normalize();
}
