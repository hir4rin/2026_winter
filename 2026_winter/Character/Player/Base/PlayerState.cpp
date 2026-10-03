#include "PlayerState.h"
#include "Player.h"
#include "../../../Math/Vector3.h"
#include "../../../Camera/Camera.h"
#include "../../../Camera/CameraManager.h"
#include "../../../Camera/CameraState/CameraStateBase.h"
#include "../../../Game.h"
#include "../Stage/Stage.h"
//#include "../Character/CharacterBase.h"

namespace
{
	constexpr float kWallCheckDistance = 50.0f;//壁判定の距離
	constexpr float kWallThreshold = 0.5f;//壁判定の法線のY成分の閾値//Yが大きいものは床か天井なので、壁扱いしない
}


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

bool PlayerState::CheckWall()
{
	auto player = m_owner.lock();
	if (!player) return false;
	auto stage = player->m_stage.lock();
	if (!stage)return false;
	auto stageHandle = stage->GetStageModelHandle();


	//Hit情報を更新//初期化
	player->m_wallHitInfo = { false, Vector3(0, 0, 0), Vector3(0, 0, 0) };


	//playerのrayを飛ばし、壁に当たったかどうかを判定する
	Vector3 playerForward = player->m_targetVec;
	playerForward.y = 0.0f;
	playerForward = playerForward.Normalize();
	Vector3 rayVec = playerForward * kWallCheckDistance;//今はこの距離が短すぎるから壁に向き合う形じゃないと当たってる判定にならない

	//playerの座標からrayVecの方向にrayを飛ばす
	Vector3 rayStart = player->GetRigidBody().GetPos() + Vector3(0, 10, 0);//上にずらしすぎたら、壁キックができなくなるので注意
	Vector3 rayEnd = rayStart + rayVec;

	auto hit = MV1CollCheck_Line(stageHandle, -1,
	rayStart.ToDxLibVector(), rayEnd.ToDxLibVector());

	//何にも当たらなかった
	if (!hit.HitFlag)
	{
		return false;
	}

	//法線のYが大きいものは床か天井なので、壁扱いしない
	if (abs(hit.Normal.y) >= kWallThreshold)
	{
		return false;
	}

	//壁キックした壁と同じ壁に当たった場合は、壁キックできないようにする
	if (player->m_lastKickWallNormal.Dot(Vector3::FromDxLibVector(hit.Normal)) > 0.9f)//数字は適当
	{
		return false;
	}

	//ここまで来たら当たっている
	//ヒット情報を更新
	player->m_wallHitInfo = { true,Vector3::FromDxLibVector(hit.Normal),Vector3::FromDxLibVector(hit.Position[0])};


	return true;
}
