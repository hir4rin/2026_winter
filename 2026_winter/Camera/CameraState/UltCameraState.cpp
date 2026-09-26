#include "UltCameraState.h"
#include "../Camera/CameraManager.h"
#include "PlayerFollowCamera.h"
#include "LockOnCameraState.h"
#include "../Camera/LockOnManager.h"
#include "Player.h"
#include "../System.h"
#include "../../BattleManager.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../../Math/Matrix4x4.h"
#include <algorithm>


namespace
{
	constexpr float kToPlayerLength = 700.0f;//プレイヤーからカメラまでの距離
	constexpr float kCameraHeightFloat = 300.0f;//カメラの高さ
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//カメラの高さ(Vector3)

	const float kUltDistance = kToPlayerLength / 2.0f;
	//const float kUltDistance = kToPlayerLength   * 2.0f;
	constexpr float kRatioCheckDistance = 800.0f;//プレイヤーの最高到達点//カメラのターゲットの割合注視点//XZ軸

	constexpr float kTargetRatioMin = 0.5f;//注視点の割合の最小値
	constexpr float kTargetRatioMax = 0.6f;//注視点の割合の最大値

	constexpr float kEtoPVecLength = 300.0f;//敵→プレイヤーベクトルの長さ
	constexpr float kRotateAngle = DX_PI_F / 2.0f;//カメラを回転させる角度

	constexpr float kBlendDuration = 15.0f;//ブレンドにかけるフレーム数
	constexpr float kBlendEasingPower = 0.5f;//ブレンドのイージング指数
}

UltCameraState::UltCameraState(std::weak_ptr<CameraManager> owner):CameraStateBase(owner)
{
}

UltCameraState::~UltCameraState()
{
}

void UltCameraState::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	ResetBlend(data.pos, data.target);
}

void UltCameraState::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto enemy = cameraManager->GetAttackTarget();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!enemy)


	{
		//敵がいない場合は、PlayerCameraに切り替える//このカメラが一番優先度高いとき
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}
	if (!player)return;

	//目標ターゲットを計算
	FixCameraPos();
	//まず回転角度をlerpする
	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();


	//注視点
	m_goalTarget = (playerPos + enemyPos) / 2 + kCameraHeight;
	//注視点の割合を決める//あんまり気に入ってない//なんか変だから値を小さくしている
	enemyPos.y = playerPos.y = 0.0f;
	float dis = (enemyPos - playerPos).Magnitude();
	//割合を決める
	float ratio = (dis - kRatioCheckDistance) / kRatioCheckDistance;
	ratio = std::clamp(ratio, kTargetRatioMin, kTargetRatioMax);
	m_goalTarget = playerPos + (enemyPos - playerPos) * ratio + kCameraHeight;

	//Blend中はBlendのほうのlerp
	if (IsBlending())
	{
		//他のカメラから遷移してきた直後:durationで指定した、ゆっくりしたLerp
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	//Blend中ではない
	else
	{
		//通常時:今まで通り、生の計算値をそのまま使う
		m_pos = m_goalPos;
		m_target = m_goalTarget;
	}

	//このカメラが一番優先度が高いときにウルトがfalseになったら、PlayerCameraに切り替える
	bool isUlt = System::GetInstance().GetBattleMgr()->GetIsUltimating();
	if (!isUlt)
	{
		bool isLockOn = cameraManager->IsLockOn();
		if (isLockOn)
		{
			cameraManager->ChangeState(std::make_shared<LockOnCameraState>(m_owner));
			return;
		}
		else
		{
			cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
			return;
		}
	}
}

void UltCameraState::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void UltCameraState::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto enemy = cameraManager->GetAttackTarget();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!enemy)return;
	if (!player)return;

	//auto rotX = Matrix4x4::MakeRotationX(0.16f);//固定

	//固定//そのまま
	//Vector3 PtoCVec = (m_pos - player->GetPos());
	//float cameraToPlayerLength = PtoCVec.Magnitude();
	//m_VecLength = cameraToPlayerLength;

	//カメラの座標を算出
	//PtoEVecの逆ベクトルを15度程度ずらす、playerの座標から足す
	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();
	//y座標を0にする
	playerPos.y = enemyPos.y = 0.0f;

	Vector3 EtoPVec = (playerPos - enemyPos).Normalize();
	EtoPVec *= kEtoPVecLength;
	//EtoPVecを90度回転させたベクトルとMainCtoPVecの内積が正か負かでどちらに回転させるかを決める
	Vector3 upVec = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 rotateBase = EtoPVec.Cross(upVec).Normalize();
	Vector3 PtoMainCVec = (cameraManager->GetActiveCamera()->GetPos() - playerPos).Normalize();
	float dot = rotateBase.Dot(PtoMainCVec);
	float angle = 0.0f;
	if (dot >= 0.0f)
	{
		angle = -kRotateAngle;
	}
	else
	{
		angle = kRotateAngle;
	}

	//水平方向の回転//敵との距離によってこの角度を帰る
	auto rotY = Matrix4x4::MakeRotationY(angle);

	
	//敵の距離によって血殺の距離が変わるのが渋いかもしれない

	//DxLibに変換
	auto EtoPVecDx = EtoPVec.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する
	//auto rotXMat = Matrix4x4::ToDxLibMatrix(rotX);//回転行列を転置する

	//カメラからプレイヤーVec
	auto RotPtoC = VTransform(EtoPVecDx, rotYMat);//回転させる
	//RotPtoC = VTransform(RotPtoC, rotXMat);//回転させる
	RotPtoC = VAdd(RotPtoC, kCameraHeight.ToDxLibVector());
	//水平方向はその向き、垂直は初期化
	m_angleH = atan2f(RotPtoC.x, RotPtoC.z) + DX_PI_F;
	//m_angleV;


	auto pos = VAdd(RotPtoC, player->GetRigidBody().GetPos().ToDxLibVector());//プレイヤーの座標に足す

	m_goalPos = Vector3::FromDxLibVector(pos);
}

void UltCameraState::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting UltCameraState::GetBlendSetting() const
{
	auto cameraManager = m_owner.lock();
	auto enemy = cameraManager ? cameraManager->GetAttackTarget() : nullptr;


	return BlendSetting{
		.mode = BlendSetting::Mode::Slerp,
		.duration = kBlendDuration,
		.easingPower = kBlendEasingPower,
		.pivot = (enemy) ? enemy->GetRigidBody().GetPos() : Vector3()
	};
}
