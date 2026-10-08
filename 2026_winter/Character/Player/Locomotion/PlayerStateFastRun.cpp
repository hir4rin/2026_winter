#include "PlayerStateFastRun.h"
#include "Player.h"
#include "../../../Input.h"
#include "../../../Camera/Camera.h"
#include "../../../Game.h"
#include "../../../System.h"

namespace
{
	constexpr float kTitleModeWalkAnimSpeed = 1.15f;//タイトル画面での歩きアニメーションの再生速度
	constexpr float kAccelFrame = 15.0f;//止まった状態からダッシュの最高速になるまでのフレーム数//Moveより長めにして、グッと加速する感じを出す
	constexpr float kAccel = Game::kRunSpeed / kAccelFrame;//1フレームに増える速さ
}

PlayerStateFastRun::PlayerStateFastRun(std::weak_ptr<Player> player):
	PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateFastRun::~PlayerStateFastRun()
{
}

void PlayerStateFastRun::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//animationの初期化
	if (player->m_isTitleMode)
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Walk"), true, kTitleModeWalkAnimSpeed);
	}
	else
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Run"), true, 0.55f);
	}
	//切り替わったフレームも止まらないように、今の速度を入れておく(前のステートで0にされているため)
	player->m_rb.m_vel = player->m_rb.m_moveVel;
	//System::GetInstance().GetSoundManager().PlaySELoop("WalkSE");
}

void PlayerStateFastRun::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();
	//Move中は上下差がないので、毎フレームすべて初期化
	player->m_rb.m_vel = Vector3(0, 0, 0);
	//鴉状態の更新
	if (input.IsPressed("LB"))
	{
		player->m_isRaven = true;
	}
	else
	{
		player->m_isRaven = false;
	}

	//暗殺
	if (input.IsTriggered("Y") && player->CanAssasin())
	{
		player->ChangeState(std::make_shared<PlayerStateAssasin>(m_owner));
		return;
	}

	//確殺
	if (input.IsTriggered("Y") && player->CanPartBrokenFinish())
	{
		player->ChangeState(std::make_shared<PlayerStatePartBrokenKill>(m_owner));
		return;
	}

	//回避
	if (input.IsTriggered("B"))
	{
		player->ChangeState(std::make_shared<PlayerStateDodge>(m_owner));
		return;
	}

	if (!input.IsLeftStickInput())
	{
		//移動状態に遷移する
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
		return;
	}
	//スキル攻撃
	if (input.IsTriggered("LB"))
	{
		if (player->CanSkillAttack())
		{
			player->ChangeState(std::make_shared<PlayerStateSkillAttack>(m_owner));
			return;
		}
	}

	//攻撃状態に遷移する
	if (input.IsTriggered("X"))//弱攻撃
	{
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::lightAttack));
		return;
	}	
	if (input.IsTriggered("Y"))//強攻撃
	{
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::heavyAttack));
		return;
	}

	//ジャンプ
	if (input.IsTriggered("A"))
	{
		player->ChangeState(std::make_shared<PlayerStateJump>(m_owner));
		return;
	}


	//アニメーションの更新
	player->m_anim.Update();
	//移動処理
	Move(input);
}

void PlayerStateFastRun::Exit()
{
	//System::GetInstance().GetSoundManager().StopSE("WalkSE");
}

void PlayerStateFastRun::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:FastRun");
}

void PlayerStateFastRun::Move(Input& input)
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;	
	//関数化したところ
	////移動方向の初期化//毎フレーム、カメラからプレイヤーへのベクトルを求めて、移動方向を決める
	//{
	//	//前後移動を最初に決める
	//	Vector3 CameraToPlayer = player->m_rb.m_pos - (player->m_camera->GetCameraPos());//カメラからプレイヤーへのベクトル
	//	//初期化
	//	Vector3 VelSize = CameraToPlayer.Normalize();//カメラからプレイヤーへのベクトルを正規化して、移動速度を5にする
	//	VelSize.y = 0.0f;//y成分は移動に関係ないので、0にする

	//	player->forward = VelSize.Normalize();
	//	player->down = player->forward * -1.0f;
	//	player->left = player->forward.Cross(Vector3(0, 1, 0)).Normalize();
	//	player->right = player->left * -1.0f;
	//	//攻撃中のコンボ後の方向入力を検知
	//	//AttackAngleInput(input);
	//}


	HandlerInput();
	//移動入力をとる
	Vector3 inputDir;
	if (input.IsPressed("Up"))
	{
		inputDir += player->forward;
	}
	if (input.IsPressed("Down"))
	{
		inputDir += player->down;
	}
	if (input.IsPressed("Right"))
	{
		inputDir += player->right;
	}
	if (input.IsPressed("Left"))
	{
		inputDir += player->left;
	}

	//移動している間は目標のベクトルを更新する//向きは入力にすぐ合わせる
	player->m_targetVec = inputDir.Normalize();
	//入力の向きにダッシュの最高速で進みたい//Moveの速さからだんだん近づける
	UpdateMoveVel(player->m_targetVec * Game::kRunSpeed, kAccel);

}
