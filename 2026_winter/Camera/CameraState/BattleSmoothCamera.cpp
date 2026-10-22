#include "BattleSmoothCamera.h"
#include "../Camera/CameraManager.h"
#include "PlayerFollowCamera.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../../Math/Matrix4x4.h"
#include <algorithm>


namespace
{
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//カメラの高さ(Vector3)//PlayerFollowCameraと同じ値

	constexpr float kPtoETargetLength = 50.0f;//プレイヤー→敵方向に注視点をずらす距離

	constexpr float kEtoPVecLength = 350.0f;//敵→プレイヤーベクトルの長さ
	constexpr float kRotateAngle = DX_PI_F / 3.0f;//カメラを回転させる角度

	constexpr float kBlendDuration = 90.0f;//ブレンドにかけるフレーム数
	constexpr float kBlendEasingPower = 0.8f;//ブレンドのイージング指数

	constexpr int kExitMoveFrame = 30;//歩き続けたら戻るフレーム数
	constexpr float kExitDistance = 800.0f;//敵との水平距離がこれより遠いと戻る
	constexpr float kExitNearDistance = 100.0f;//空中の敵との水平距離がこれより近いと戻る
}

BattleSmoothCamera::BattleSmoothCamera(std::weak_ptr<CameraManager> owner):CameraStateBase(owner)
{
}

BattleSmoothCamera::~BattleSmoothCamera()
{
}

void BattleSmoothCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	ResetBlend(data.pos, data.target);
	m_moveFrame = 0;
	//遷移元のカメラ位置(m_pos)から、近いほうの45度を決める
	DecideRotateSide();
}

void BattleSmoothCamera::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto enemy = cameraManager->GetAttackTarget();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!enemy)
	{
		//敵がいない場合は、PlayerCameraに切り替える
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}
	if (!player)return;

	//右スティックの入力があったら、PlayerCameraに戻す
	auto rightStick = Input::GetInstance().GetRightStickInput();
	if (rightStick.x != 0.0f || rightStick.y != 0.0f)
	{
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();

	//歩き出した・離れた・空中の敵が近すぎるときもPlayerCameraに戻す
	if (IsExitCondition(playerPos, enemyPos, enemy->IsFloor()))
	{
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}

	//目標座標を計算
	FixCameraPos();

	enemyPos.y = playerPos.y;

	//注視点//プレイヤーから敵のほうへ固定距離ずらす//高さはPlayerFollowCameraと同じ
	Vector3 pToEVec = (enemyPos - playerPos).Normalize() * kPtoETargetLength;
	//Vector3 pToEVec = (enemyPos - playerPos) * 0.45f;
	m_goalTarget = (playerPos + pToEVec) + kCameraHeight;
	//m_goalTarget = (playerPos +pToEVec) + kCameraHeight;

	//Blend中はBlendのほうのlerp
	if (IsBlending())
	{
		//他のカメラから遷移してきた直後:durationで指定した、プレイヤー中心のSlerp
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	//Blend中ではない
	else
	{
		//通常時:生の計算値をそのまま使う
		m_pos = m_goalPos;
		m_target = m_goalTarget;
	}
}

void BattleSmoothCamera::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void BattleSmoothCamera::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto enemy = cameraManager->GetAttackTarget();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!enemy)return;
	if (!player)return;

	//カメラの座標を算出
	//EtoPVecをプレイヤー中心に45度回転させ、playerの座標から足す
	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();
	//y座標を0にする
	playerPos.y = enemyPos.y = 0.0f;

	Vector3 EtoPVec = (playerPos - enemyPos).Normalize();
	EtoPVec *= kEtoPVecLength;

	//水平方向の回転//どちら側に回すかはEnterで決めたものを使う
	auto rotY = Matrix4x4::MakeRotationY(m_rotateAngle);

	//DxLibに変換
	auto EtoPVecDx = EtoPVec.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する

	//プレイヤーからカメラVec
	auto RotPtoC = VTransform(EtoPVecDx, rotYMat);//回転させる
	RotPtoC = VAdd(RotPtoC, kCameraHeight.ToDxLibVector());

	auto pos = VAdd(RotPtoC, player->GetRigidBody().GetPos().ToDxLibVector());//プレイヤーの座標に足す

	m_goalPos = Vector3::FromDxLibVector(pos);
}

void BattleSmoothCamera::CameraSetting()
{
}

void BattleSmoothCamera::DecideRotateSide()
{
	m_rotateAngle = kRotateAngle;

	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto enemy = cameraManager->GetAttackTarget();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!enemy)return;
	if (!player)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();
	Vector3 cameraPos = m_pos;
	//y座標を0にする
	playerPos.y = enemyPos.y = cameraPos.y = 0.0f;

	//EtoPVecを90度回転させたベクトルとPtoCVecの内積が正か負かでどちらに回転させるかを決める
	Vector3 EtoPVec = (playerPos - enemyPos).Normalize();
	Vector3 upVec = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 rotateBase = EtoPVec.Cross(upVec).Normalize();
	Vector3 PtoCVec = (cameraPos - playerPos).Normalize();
	float dot = rotateBase.Dot(PtoCVec);
	if (dot >= 0.0f)
	{
		m_rotateAngle = -kRotateAngle;
	}
	else
	{
		m_rotateAngle = kRotateAngle;
	}
}

bool BattleSmoothCamera::IsExitCondition(const Vector3& playerPos, const Vector3& enemyPos, bool isEnemyFloor)
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return false;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return false;

	//攻撃せずに歩いている時間を数える//実フレーム
	if (player->IsMoveState())
	{
		m_moveFrame++;
	}
	else
	{
		m_moveFrame = 0;
	}
	if (m_moveFrame >= kExitMoveFrame)return true;

	//水平距離
	Vector3 toEnemy = enemyPos - playerPos;
	float horizontalDist = Vector3(toEnemy.x, 0.0f, toEnemy.z).Magnitude();

	//離れすぎ
	if (horizontalDist > kExitDistance)return true;
	//空中の敵が真上近くにいると向きが暴れる
	if (!isEnemyFloor && horizontalDist < kExitNearDistance)return true;

	return false;
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting BattleSmoothCamera::GetBlendSetting() const
{
	//プレイヤーを中心に回り込む
	auto cameraManager = m_owner.lock();
	std::shared_ptr<Player> player;
	if (cameraManager) player = cameraManager->GetContext()->m_player.lock();

	return BlendSetting{
		.mode = BlendSetting::Mode::Slerp,
		.duration = kBlendDuration,
		.easingMode = EasingMode::EaseIn,
		.easingPower = kBlendEasingPower,
		.pivot = (player) ? player->GetRigidBody().GetPos() : Vector3(),
	};
}
