#include "LockOnCameraState.h"
#include "../CameraManager.h"
#include "../LockOnManager.h"
#include "PlayerFollowCamera.h"
#include "../Character/Enemy/EnemyBase.h"
//#include "../Character/Enemy/Boss/BossEnemy.h"
#include "Player.h"
//#include "../Stage/Stage.h"
#include "../Math/Matrix4x4.h"
#include <algorithm>

namespace
{
	constexpr float kToPlayerLength = 350.0f;//プレイヤーからカメラまでの距離
	constexpr float kCameraHeightFloat = 300.0f;//カメラの高さ
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//カメラの高さ(Vector3)

	constexpr float kLockOnMaxDistance = 10000.0f;//ロックオンの最大距離
	constexpr float kGroundCheckDistance = 800.0f;//プレイヤーの最高到達点//カメラのターゲットの割合注視点//Y軸
	constexpr float kRatioCheckDistance = 800.0f;//プレイヤーの最高到達点//カメラのターゲットの割合注視点//XZ軸

	constexpr float kTargetSwitchLerpFrame = 30.0f;//ロックオン対象切り替え時、注視点をlerpするフレーム数

	constexpr float kRaticleMinDistance = 500.0f;//これ以下の距離ではレティクルを最大サイズにする
	constexpr float kRaticleMaxDistance = 5000.0f;//これ以上の距離ではレティクルを最小サイズにする
	constexpr float kRaticleMinScale = 0.05f;//最小サイズ(kRaticleMaxDistance以上離れたとき)
	constexpr float kRaticleMaxScale = 0.15f;//最大サイズ(kRaticleMinDistance以下に近づいたとき)

	constexpr float kTargetRatioMin = 0.1f;//注視点の割合の最小値
	constexpr float kTargetRatioMax = 0.5f;//注視点の割合の最大値

	constexpr float kRotateAngle = DX_PI_F / 6;//カメラを回転させる角度
	constexpr float kCameraPosOffsetY = 160.0f;//カメラ座標のY方向オフセット
	constexpr float kEnemyScreenPosOffsetY = 100.0f;//敵のスクリーン座標変換時のY方向オフセット

	constexpr float kWallMargin = 20.0f;//カメラを壁の手前に押し戻す時の余白
	constexpr float kBossReticleExtraOffsetY = 100.0f;//ボスをロックオンしている時、レティクルをさらに上げる量
}

LockOnCameraState::LockOnCameraState(std::weak_ptr<CameraManager> owner) : CameraStateBase(owner)
{
	m_raticleHandle = LoadGraph("data/UI/LockOnRaticle.png");
}

LockOnCameraState::~LockOnCameraState()
{
	DeleteGraph(m_raticleHandle);
}

void LockOnCameraState::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	ResetBlend(data.pos, data.target);
	Update();
}

void LockOnCameraState::Update()
{
	//ロックオンカメラもPlayerCaemraのように
	//プレイヤーの高さに応じて注視点の割合を決めて変える

	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto lockOnManager = cameraManager->GetLockOnManager().lock();
	std::shared_ptr<EnemyBase> enemy;
	if (lockOnManager) enemy = lockOnManager->GetTarget().lock();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//ロックオン対象が切り替わったら、注視点のlerpをやり直す
	if (enemy != m_lastTargetEnemy.lock())
	{
		m_targetLerpStart = m_target;
		m_targetLerpElapsed = 0.0f;
		m_lastTargetEnemy = enemy;
	}



	//目標ターゲットを計算
	FixCameraPos();
	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();

	float distance = (enemyPos - playerPos).Magnitude();
	//ロックオンの最大距離を超えたらロックオンを解除する
	if (distance > kLockOnMaxDistance)
	{
		//mainCamera->SetLockOn(false);
		cameraManager->SetLockOn(false);
		//解放
		//m_lockOnEnemy.reset();

		//PlayerCameraに切り替える//処理を呼ぶ
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
	}
	Vector3 targetPos = (playerPos + enemyPos) / 2;
	//注視点の割合を決める
	float dis = (enemyPos - playerPos).Magnitude();
	//割合を決める
	float ratio = (dis - kRatioCheckDistance) / kRatioCheckDistance;
	ratio = std::clamp(ratio, kTargetRatioMin, kTargetRatioMax);
	targetPos = playerPos + (enemyPos - playerPos) * ratio;

	//Stage.hが存在しないため一旦コメントアウト
	//auto stage = m_stage.lock();
	//if (stage)
	//{
	//	Vector3 endPos = targetPos + Vector3(0.0f, -kGroundCheckDistance, 0.0f);
	//	//stage地面とターゲットの距離を取得する
	//	auto hitPoly = MV1CollCheck_Line(stage->GetStageModelHandle(), -1, targetPos.ToDxLibVector(), endPos.ToDxLibVector());
	//	if (hitPoly.HitFlag)
	//	{
	//		float distance = targetPos.y - hitPoly.HitPosition.y;
	//		if (distance >= 0.0f)
	//		{
	//			//カメラ注視点の割合をきめる
	//			float ratio = distance / kGroundCheckDistance;
	//			targetPos.y *= ratio;
	//		}
	//	}
	//}


	//ターゲットの位置を更新
	m_goalTarget = targetPos + kCameraHeight;

	//Blend中はBlendのほうのlerp
	if (IsBlending())
	{
		//他のカメラから遷移してきた直後:durationで指定した、ゆっくりしたLerp
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	//Blend中ではない
	else
	{
		//通常時:座標は今まで通り、生の計算値をそのまま使う
		m_pos = m_goalPos;

		//注視点はロックオン対象切り替え時になめらかに補間する
		m_targetLerpElapsed += 1.0f;
		float lerpT = std::clamp(m_targetLerpElapsed / kTargetSwitchLerpFrame, 0.0f, 1.0f);
		m_target = Vector3::Lerp(m_targetLerpStart, m_goalTarget, lerpT);
	}
}

void LockOnCameraState::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void LockOnCameraState::Draw()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto lockOnManager = cameraManager->GetLockOnManager().lock();
	std::shared_ptr<EnemyBase> enemy;
	if (lockOnManager) enemy = lockOnManager->GetTarget().lock();
	if (!enemy)return;

	//敵の座標をスクリーン座標に変換する//ボスをロックオンしている時は、レティクルをさらに上げる
	float reticleOffsetY = kEnemyScreenPosOffsetY;
	//BossEnemy.hが存在しないため一旦コメントアウト
	//if (std::dynamic_pointer_cast<BossEnemy>(enemy))
	//{
	//	reticleOffsetY += kBossReticleExtraOffsetY;
	//}
	Vector3 enemyPos = enemy->GetRigidBody().GetPos() + Vector3(0.0f, reticleOffsetY, 0.0f);
	VECTOR enemyPos2D = ConvWorldPosToScreenPos(enemyPos.ToDxLibVector());

	//カメラの前方(画面内)にいるときだけ描画する
	if (enemyPos2D.z < 0.0f || enemyPos2D.z > 1.0f)return;


	//カメラからの敵との距離によって、レティクルの大きさを変える
	float distance = (enemy->GetRigidBody().GetPos() - m_pos).Magnitude();
	float t = (distance - kRaticleMinDistance) / (kRaticleMaxDistance - kRaticleMinDistance);
	t = std::clamp(t, 0.0f, 1.0f);
	float scale = kRaticleMaxScale + (kRaticleMinScale - kRaticleMaxScale) * t;//近いほど大きく、遠いほど小さく

	DrawRotaGraph(static_cast<int>(enemyPos2D.x), static_cast<int>(enemyPos2D.y), scale, 0.0, m_raticleHandle, TRUE);
}

void LockOnCameraState::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto lockOnManager = cameraManager->GetLockOnManager().lock();
	std::shared_ptr<EnemyBase> enemy;
	if (lockOnManager) enemy = lockOnManager->GetTarget().lock();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!enemy)return;
	if (!player)return;

	//水平方向の回転//敵との距離によってこの角度を帰る
	//現在のカメラtoプレイヤーの向きとプレイヤーto敵の向きに応じて角度を反転？
	Matrix4x4 rotY;

	//auto rotX = Matrix4x4::MakeRotationX(0.16f);//固定

	//ここも敵との距離に寄って変える
	float cameraToPlayerLength = kToPlayerLength;

	//カメラの座標を算出
	//PtoEVecの逆ベクトルを15度程度ずらす、playerの座標から足す
	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();
	//y座標を0にする
	playerPos.y = enemyPos.y = 0.0f;

	Vector3 EtoPVec = (playerPos - enemyPos).Normalize();
	EtoPVec *= cameraToPlayerLength;

	//EtoPVecを90度回転させたベクトルとMainCtoPVecの内積が正か負かでどちらに回転させるかを決める
	Vector3 upVec = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 rotateBase = EtoPVec.Cross(upVec).Normalize();//EtoPVecを90度回転させたベクトル
	Vector3 PtoMainCVec = (m_pos - playerPos).Normalize();//プレイヤーからメインカメラへのベクトル
	float dot = rotateBase.Dot(PtoMainCVec);
	if (dot >= 0.0f)
	{
		//正の時、rotateBaseの方向に回転させる
		rotY = Matrix4x4::MakeRotationY(-kRotateAngle);
	}
	else
	{
		//負の時、rotateBaseの逆方向に回転させる
		rotY = Matrix4x4::MakeRotationY(kRotateAngle);
	}
	//やり方がわからないので、一旦これで
	rotY = Matrix4x4::MakeRotationY(kRotateAngle);

	//auto CtoP = Vector3(0.0f, 0.0f, -cameraToPlayerLength);//プレイヤーからカメラへのベクトル

	//カメラの揺れを加える
	//EtoPVec = EtoPVec + CameraShakeUpdate();
	//DxLibに変換
	auto EtoPVecDx = EtoPVec.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する
	//auto rotXMat = Matrix4x4::ToDxLibMatrix(rotX);//回転行列を転置する

	//カメラからプレイヤーVec
	auto RotPtoC = VTransform(EtoPVecDx, rotYMat);//回転させる
	//RotPtoC = VTransform(RotPtoC, rotXMat);//回転させる
	RotPtoC = VAdd(RotPtoC, VGet(0.0f, kCameraPosOffsetY, 0.0f));

	//水平方向はその向き、垂直は初期化で角度を更新し、プレイヤーカメラに渡す
	//atan2f(cross,dot)で二つのベクトルの角度が出る//理解済み
	//Vector3 BaseVec = Vector3(0.0f, 0.0f, 1.0f);//Z軸の正方向//これを基準にする
	//float dot = BaseVec.Dot(RotPtoC);
	// float cross = BaseVec.Cross2DXZ(Vector3::FromDxLibVector(RotPtoC));
	//m_angleH = atan2f(cross, dot);
	//m_angleH += DX_PI_F;
	m_angleH = atan2f(RotPtoC.x, RotPtoC.z) + DX_PI_F;


	auto pos = VAdd(RotPtoC, player->GetRigidBody().GetPos().ToDxLibVector());//プレイヤーの座標に足す

	m_goalPos = Vector3::FromDxLibVector(pos);

	//カメラとオブジェクトの押し戻し判定(プレイヤー→カメラの線分とステージポリゴンの当たり判定)
	//Stage.hが存在しないため一旦コメントアウト
	//auto stage = m_stage.lock();
	//if (stage)
	//{
	//	Vector3 lineStart = player->GetRigidBody().GetPos() + kCameraHeight;//playerPosはこの関数内でy=0にされているため使わない
	//	auto hitPoly = MV1CollCheck_Line(stage->GetStageModelHandle(), -1, lineStart.ToDxLibVector(), m_goalPos.ToDxLibVector());
	//	if (hitPoly.HitFlag)
	//	{
	//		//当たった位置から、壁にめり込まないよう少し手前に戻す
	//		Vector3 hitPos = Vector3::FromDxLibVector(hitPoly.HitPosition);
	//		Vector3 dir = (m_goalPos - lineStart).Normalize();
	//		m_goalPos = hitPos - dir * kWallMargin;
	//	}
	//}
}

void LockOnCameraState::CameraSetting()
{
}
