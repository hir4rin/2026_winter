#include "PlayerState.h"
#include "Player.h"
#include "../../../Math/Vector3.h"
#include "../../../Camera/Camera.h"
#include "../../../Camera/CameraManager.h"
#include "../../../Camera/CameraState/CameraStateBase.h"
#include "../../../Game.h"

PlayerState::PlayerState(std::weak_ptr<Player> owner) :
	m_owner(owner)
{
}

void PlayerState::HandlerInput()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;

	//タイトル画面ではカメラが追従しない(固定カメラの)ため、カメラ基準ではなくワールド座標の固定軸を使う
	if (player->m_isTitleMode)
	{
		player->forward = Vector3(0, 0, 1);
		player->down = Vector3(0, 0, -1);
		player->left = Vector3(-1, 0, 0);
		player->right = Vector3(1, 0, 0);
		return;
	}

	//移動方向の初期化//毎フレーム、カメラからプレイヤーへのベクトルを求めて、移動方向を決める
	{
		//前後移動を最初に決める
		Vector3 CameraToPlayer = player->m_rb.m_pos - (player->m_cameraManager.lock()->GetActiveCamera()->GetPos());//カメラからプレイヤーへのベクトル
		//Vector3 CameraToPlayer = player->m_rb.m_pos - (player->m_camera->GetCameraPos());//カメラからプレイヤーへのベクトル
		//初期化
		Vector3 VelSize = CameraToPlayer.Normalize();//カメラからプレイヤーへのベクトルを正規化して、移動速度を5にする
		VelSize.y = 0.0f;//y成分は移動に関係ないので、0にする

		player->forward = VelSize.Normalize();
		player->down = player->forward * -1.0f;
		player->left = player->forward.Cross(Vector3(0, 1, 0)).Normalize();
		player->right = player->left * -1.0f;
		//攻撃中のコンボ後の方向入力を検知
		//AttackAngleInput(input);
	}
}

void PlayerState::ClampSpeed()
{
	//x,z成分での速度制限
	auto player = m_owner.lock();
	if (!player) return;
	Vector3 velXZ = Vector3(player->m_rb.m_vel.x, 0, player->m_rb.m_vel.z);
	if (velXZ.Magnitude() > Game::kAirMaxSpeed)
	{
		velXZ = velXZ.Normalize() * Game::kAirMaxSpeed;
		player->m_rb.m_vel.x = velXZ.x;
		player->m_rb.m_vel.z = velXZ.z;
	}
}