#include "PlayerStateWallRun.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../../System.h"

namespace
{
	constexpr float kWallRunSpeed = 8.0f;//壁走りの速度
	constexpr float kWallRunTilt = DX_PI_F *  1/ 6;//壁走り中のモデルの傾き//足を壁側に向ける
	constexpr float kWallRunGap = 0.1f;//壁とカプセルの間のすき間//食い込み防止
}

PlayerStateWallRun::PlayerStateWallRun(std::weak_ptr<Player> player) : PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateWallRun::~PlayerStateWallRun()
{
}

void PlayerStateWallRun::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//走る方向を決める//壁の法線方向に対して、左か右かで決める
	Vector3 CrossWallNormVec = player->m_wallHitInfo.wallNormal.Cross(Vector3(0, 1, 0));

	//y軸を消す
	CrossWallNormVec.y = 0.0f;
	//このベクトルと入力の内積で、左か右かを判定する

	//入力方向(カメラ基準)//Jump/FallのMoveと同じ作り方
	HandlerInput();
	Vector3 inputDir = Vector3(0, 0, 0);
	if (input.IsPressed("Up"))    inputDir += player->forward;
	if (input.IsPressed("Down"))  inputDir += player->down;
	if (input.IsPressed("Right")) inputDir += player->right;
	if (input.IsPressed("Left"))  inputDir += player->left;

	//入力がなかったら、プレイヤーの方向を使う
	if(inputDir.sqMagnitude() < 0.01f)
	{
		inputDir = player->m_targetVec;
	}
	else
	{
		inputDir = inputDir.Normalize();
	}

	//内積
	float dot = CrossWallNormVec.Dot(inputDir);
	if(dot > 0.0f)
	{
		m_wallRunDirVec = CrossWallNormVec.Normalize();
		m_wallRunDir = WallRunDir::Right;
	}
	else
	{
		m_wallRunDirVec =  CrossWallNormVec.Normalize() * -1.0f;
		m_wallRunDir = WallRunDir::Left;
	}

	//壁の位置にプレイヤーの足元を合わせる//高さはそのまま
	//壁ぴったりだとカプセルが食い込んで進めないので、法線方向に半径+すき間だけ離す
	Vector3 wallNormalXZ = player->m_wallHitInfo.wallNormal;
	wallNormalXZ.y = 0.0f;
	wallNormalXZ = wallNormalXZ.Normalize();
	Vector3 fitPos = player->m_wallHitInfo.hitPos + wallNormalXZ * (player->GetRadius() + kWallRunGap);
	player->m_rb.m_pos.x = fitPos.x;
	player->m_rb.m_pos.z = fitPos.z;

	//足が壁側を向くようにモデルを傾ける
	//Rightのときは壁がモデルのローカル+X側にあるので+、Leftは逆
	float tiltSign = (m_wallRunDir == WallRunDir::Right) ? 1.0f : -1.0f;
	player->SetTargetTilt(kWallRunTilt * tiltSign);

	player->m_rb.m_vel = m_wallRunDirVec * kWallRunSpeed;
	//playerの向きを進む方向にする
	player->m_targetVec = m_wallRunDirVec;

	//アニメーションの切り替え
	player->m_anim.ChangeAnim(player->GetAnimName("Walk"), true, 0.5f);
}

void PlayerStateWallRun::Update()
{
	//weak_ptrからshared_ptrを取得する
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();


	player->m_rb.m_vel = m_wallRunDirVec * kWallRunSpeed;


	//前方向の壁とぶつかったらの処理がいる

	//壁がなくなったら、または壁走りゾーンから出たら、同じように落下状態に移行する
	if (!CheckNextFrameWall() || !IsInWallZone(Collider::ColRole::WallRunZone))
	{
		//今は下と同じだけど、必要であればあのふわっとジャンプを実装する

		player->ChangeState(std::make_shared<PlayerStateWallRunKick>(m_owner));
		return;
	}


	if(input.IsTriggered("A"))
	{
		player->ChangeState(std::make_shared<PlayerStateWallRunKick>(m_owner));
		return;
	}

	//アニメーション
	player->m_anim.Update();
}

void PlayerStateWallRun::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;
	//傾きを戻す//lerpなのでなめらかに起き上がる
	player->SetTargetTilt(0.0f);
}

void PlayerStateWallRun::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:WallRun");
#endif
}
