#include "PlayerStateAirAssasin.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"
#include "Main/System.h"
#include "../Camera/CameraManager.h"


namespace
{
	//接近でたどり着く敵までの距離
	constexpr float kApproachDistance = 10.0f;
	//接近のフレーム
	constexpr float kApproachFrame = 11.0f;
	//一緒に移動する速度
	constexpr float kMoveSpeed = 24.0f;
	//一緒に移動するフレーム
	constexpr float kMoveFrame = 14.0f;//11+14=25F
	//頭破壊のフレーム
	constexpr float kExecuteFrame = 19.0f;//44F
}

PlayerStateAirAssasin::PlayerStateAirAssasin(std::weak_ptr<Player> player) :PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateAirAssasin::~PlayerStateAirAssasin()
{
}

void PlayerStateAirAssasin::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//空中暗殺:Assasin02->fromAvobeForwardVictim
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Assasin02"), false, 0.7f);
	//スタートする
	m_state = AssasinState::Approach;

	//敵のステートを変える
	auto assasinTarget = player->GetAssasinTarget();
	if (!assasinTarget)return;
	assasinTarget->OnAirAssasined();

	//暗殺対象を固定する
	player->SetIsAssasinating(true);

	//キャラ同士の押し戻しを有効化
	player->SetIsGhost(true);

	//敵のほうを向く
	Vector3 toEnemy = assasinTarget->GetRigidBody().GetPos() - player->GetRigidBody().GetPos();
	player->m_targetVec = toEnemy.Normalize();

	//カメラを切り替える
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;
	cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::AirAssasinCameraStart);


	System::GetInstance().SetTimeScale(0.7f);
}

void PlayerStateAirAssasin::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto assasinTarget = player->GetAssasinTarget();
	if (!assasinTarget)return;
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = assasinTarget->GetRigidBody().GetPos();

	Vector3 toPlayer = playerPos - enemyPos;
	toPlayer = toPlayer.Normalize();
	toPlayer.y = 0;

	m_timer += 1.0f * System::GetInstance().GetTimeScale();

	switch (m_state)
	{
	case AssasinState::Approach:
	{
		//敵のそばまでlerpで近寄る
		Vector3 targetPos = enemyPos + toPlayer * kApproachDistance;
		float t = m_timer / kApproachFrame;

		//敵との間にオブジェクトがあったら、、の処理がいると思う

		player->m_rb.m_pos = Vector3::Lerp(playerPos, targetPos, t);
		if (m_timer > kApproachFrame)
		{
			m_state = AssasinState::Move;
			m_timer = 0.0f;
			System::GetInstance().SetTimeScale(0.7f);
		}
	}
	break;
	case AssasinState::Move:
		//敵と一緒に水平移動
		player->m_rb.m_vel = toPlayer * -1 * kMoveSpeed;
		if (m_timer > kMoveFrame)
		{
			m_state = AssasinState::Execute;
			m_timer = 0.0f;
		}
		break;
	case AssasinState::Execute:
		//止まる
		player->m_rb.m_vel = Vector3(0.0f, 0.0f, 0.0f);
		if (m_timer > kExecuteFrame)
		{
			m_state = AssasinState::End;
			cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::AirAssasinCamera);
			System::GetInstance().SetTimeScale(1.0f);
		}
		break;
	case AssasinState::End:
		break;
	}


	if (player->m_anim.GetAnimEndFlag())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
		return;
	}
	player->m_anim.Update();
}

void PlayerStateAirAssasin::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;

	//キャラ同士の押し戻しを有効化
	player->SetIsGhost(false);
	//地面に着地している
	player->SetIsFloor(true);

	//暗殺対象の固定を解除する
	player->SetIsAssasinating(false);
	//ターゲットをリセット
	player->ClearAssasinTarget();
}

void PlayerStateAirAssasin::DebugDraw()
{
}
