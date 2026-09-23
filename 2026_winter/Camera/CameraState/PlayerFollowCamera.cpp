#include "PlayerFollowCamera.h"
#include "../Cameramanager.h"
#include "Player.h"
//#include "../PlayerCamera.h"
#include "../Math/Matrix4x4.h"
#include "../Input.h"
#include "../CameraManager.h"
#include "../LockOnManager.h"
#include "../Character/Enemy/EnemyBase.h"
//#include "../Stage/Stage.h"
#include "../System.h"
#include "../BattleManager.h"
#include <algorithm>



namespace
{
	//constexpr int kNormalRx = 128;//右スティックの通常の値

	constexpr float kToPlayerLength = 700.0f;//プレイヤーからカメラまでの距離
	constexpr float kCameraHeightFloat = 300.0f;//カメラの高さ
	constexpr float kCameraAngleSpeed = 0.03f;//カメラの回転速度
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//カメラの高さ(Vector3)

	constexpr float kRockOnMaxDistance = 10000.0f;//ロックオンの最大距離

	constexpr float kGroundCheckDistance = 800.0f;//プレイヤーの最高到達点//カメラのターゲットの割合注視点

	constexpr float kToPlayerLengthScale = 0.5f;//プレイヤーからカメラまでの距離にかける倍率
	constexpr float kCameraLerpFactor = 0.5f;//カメラの位置・注視点のラープ係数
	constexpr float kGroundCheckRayStartHeight = 250.0f;//地面判定のレイを飛ばす開始位置の高さ

	constexpr float kAngleFullTurn = DX_PI_F * 2.0f;//水平角度の一周分
	constexpr float kVerticalAngleSpeedDivisor = 3.0f;//垂直方向の回転速度にかける除数
	constexpr float kAngleVUpperMultiplier = 0.5f;//垂直角度上限の倍率
	constexpr float kAngleVLowerMultiplier = 0.4f;//垂直角度下限の倍率
	constexpr float kAngleVLimitOffset = 0.6f;//垂直角度上下限のオフセット

	constexpr float kWallMargin = 20.0f;//カメラを壁の手前に押し戻す時の余白
}

PlayerFollowCamera::PlayerFollowCamera(std::weak_ptr<CameraManager> owner):CameraStateBase(owner)
{
}

PlayerFollowCamera::~PlayerFollowCamera()
{
}

void PlayerFollowCamera::Enter(CameraData data)
{
	auto player = m_owner.lock()->GetContext()->m_player.lock();

	Vector3 playerPos = Vector3(player->GetRigidBody().GetPos().x, 0.0f, player->GetRigidBody().GetPos().z);

	//カメラの位置を調整する
	FixCameraPos();

	m_target = playerPos + kCameraHeight;
	//座標をセット
	//m_target = data.target;
	if (data.overrideSetting.mode == BlendSetting::Mode::None)
	{
		//上書きではない遷移なら書き換える
		m_angleH = data.angleH;
		m_angleV = data.angleV;
	}
	//指定がある遷移
	else
	{
		//指定があるときは変えない
	}
	


	ResetBlend(data.pos, data.target);
	//もしあるなら上書きする
	if (data.overrideSetting.mode != BlendSetting::Mode::None)
	{
		m_activeBlend = data.overrideSetting;
	}


	//カメラの位置と注視点を反映する
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLibVector(), m_target.ToDxLibVector());
}

void PlayerFollowCamera::Update()
{
	auto player = m_owner.lock()->GetContext()->m_player.lock();

	InputRightStick();

	//ターゲットの位置を更新
	Vector3 playerPos = player->GetRigidBody().GetPos();
	m_testPos = playerPos;
	//プレイヤーの位置と直下の地面との距離が一定以下の場合、カメラを動かさない
	Vector3 endPos = playerPos + Vector3(0.0f, -kGroundCheckDistance, 0.0f);
	m_testPos2 = endPos;
	//Stage.hが存在しないため一旦コメントアウト
	//auto stage = m_stage.lock();
	//if (stage)
	//{
	//	//ちょっと上から判定
	//	Vector3 startPos = playerPos + Vector3(0, kGroundCheckRayStartHeight, 0);
	//	//stage地面とプレイヤーの距離を取得する
	//	auto hitPoly = MV1CollCheck_Line(stage->GetStageModelHandle(), -1, startPos.ToDxLibVector(), endPos.ToDxLibVector());
	//	if (hitPoly.HitFlag)
	//	{
	//		float distance = playerPos.y - hitPoly.HitPosition.y;
	//		if (distance >= 0.0f)
	//		{
	//			//カメラ注視点の割合をきめる
	//			float ratio = distance / kGroundCheckDistance;
	//			playerPos.y *= ratio;
	//		}
	//		m_testPos2 = playerPos;
	//	}
	//}
	//m_testPos2 = endPos;

	//rayVecを更新
	m_rayVec = playerPos - m_pos;
	//rayVecを正規化
	m_rayVec = m_rayVec.Normalize();
	//敵の座標との射影ベクトルを求める


	//playerPos.y = 0.0f;//プレイヤーのy座標は0にする
	m_goalTarget = playerPos + kCameraHeight;
	m_target = Vector3::Lerp(m_target, m_goalTarget, kCameraLerpFactor);

	//カメラの位置を調整する
	FixCameraPos();

	//大きさによって、ラープかどうかを変える
	//Vector3 dis = m_goalPos - m_pos;
	//if (dis.Magnitude() < 0.15f)
	//{
	//	m_pos = m_goalPos;
	//}

	//Lerpの割合を上下差がある攻撃で変えたりするとよい

	//Blend中はBlendのほうのlerp
	if (IsBlending())
	{
		//他のカメラから遷移してきた直後:durationで指定した、ゆっくりしたLerp
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	//Blend中ではない
	else
	{
		//イベント発火中なら元に戻す
		if (System::GetInstance().GetBattleMgr()->GetIsEventPlaying())
		{
			System::GetInstance().GetBattleMgr()->SetIsEventPlaying(false);
		}

		//通常時:今まで通りの追従Lerp(ジャンプなどの滑らかな追従はここで維持)
		m_pos = Vector3::Lerp(m_pos, m_goalPos, kCameraLerpFactor);
		m_target = Vector3::Lerp(m_target, m_goalTarget, kCameraLerpFactor);
	}


	//カメラの位置と注視点を反映する
	//SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLibVector(), m_target.ToDxLibVector());

}

void PlayerFollowCamera::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void PlayerFollowCamera::FixCameraPos()
{
	//水平方向の回転
	auto rotY = Matrix4x4::MakeRotationY(m_angleH);
	auto rotX = Matrix4x4::MakeRotationX(m_angleV);

	//本当はこの回転行列はベクトルの量、角度が固定なのでOKだが、変わると回転量が変わるので危ないあぶない
	float cameraToPlayerLength = kToPlayerLength * kToPlayerLengthScale;

	//カメラの座標を算出
	auto CtoP = Vector3(0.0f, 0.0f, -cameraToPlayerLength);//プレイヤーからカメラへのベクトル
	//DxLibに変換
	auto CtoPVec = CtoP.ToDxLibVector();
	auto rotYMat = rotY.ToDxLibMatrix(rotY);//回転行列を転置する
	auto rotXMat = rotX.ToDxLibMatrix(rotX);//回転行列を転置する
	//カメラからプレイヤーVec
	auto RotCtoP = VTransform(CtoPVec, rotXMat);//回転させる
	RotCtoP = VTransform(RotCtoP, rotYMat);//回転させる

	auto pos = VAdd(RotCtoP, m_goalTarget.ToDxLibVector());//プレイヤーの座標に足す

	m_goalPos = Vector3::FromDxLibVector(pos);

	//カメラとオブジェクトの押し戻し判定(注視点→カメラの線分とステージポリゴンの当たり判定)
	//Stage.hが存在しないため一旦コメントアウト
	//auto stage = m_stage.lock();
	//if (stage)
	//{
	//	auto hitPoly = MV1CollCheck_Line(stage->GetStageModelHandle(), -1, m_goalTarget.ToDxLibVector(), m_goalPos.ToDxLibVector());
	//	if (hitPoly.HitFlag)
	//	{
	//		//当たった位置から、壁にめり込まないよう少し手前に戻す
	//		Vector3 hitPos = Vector3::FromDxLibVector(hitPoly.HitPosition);
	//		Vector3 dir = (m_goalPos - m_goalTarget).Normalize();
	//		m_goalPos = hitPos - dir * kWallMargin;
	//	}
	//}
}

void PlayerFollowCamera::CameraSetting()
{
	//FixCameraPos();
	//カメラの位置と注視点を反映する
	SetCameraPositionAndTarget_UpVecY(m_pos.ToDxLibVector(), m_target.ToDxLibVector());
}




void PlayerFollowCamera::InputRightStick()
{
	if (GetJoypadXInputState(DX_INPUT_PAD1, &xi) == 0)//右スティックの入力を取得できたら
	{
		//右スティックの範囲は-32768~35767
		short rx = xi.ThumbRX;
		short ry = xi.ThumbRY;
		//デッドゾーンの設定
		const int DeadZone = 8000;//スティックの傾きがこの値以下の場合は、傾いていないとみなす
		if (abs(rx) < DeadZone)
		{
			rx = 0;
		}
		if (abs(ry) < DeadZone)
		{
			ry = 0;
		}
		//スティックの傾きに応じてカメラを回転させる
		if (rx > 0)//右に傾いている
		{
			m_angleH += kCameraAngleSpeed;
			if (m_angleH > kAngleFullTurn)
			{
				m_angleH -= kAngleFullTurn;
			}
		}
		else if (rx < 0)//左に傾いている
		{
			m_angleH -= kCameraAngleSpeed;
			if (m_angleH < -kAngleFullTurn)
			{
				m_angleH += kAngleFullTurn;
			}
		}
		if (ry < 0)//下に傾いている
		{
			m_angleV += kCameraAngleSpeed * 1 / kVerticalAngleSpeedDivisor;
			// ある一定角度以上にはならないようにする
			if (m_angleV > DX_PI_F * kAngleVUpperMultiplier - kAngleVLimitOffset)
			{
				m_angleV = DX_PI_F * kAngleVUpperMultiplier - kAngleVLimitOffset;
			}

		}
		else if (ry > 0)//上に傾いている
		{
			m_angleV -= kCameraAngleSpeed * 1 / kVerticalAngleSpeedDivisor;
			// ある一定角度以下にはならないようにする
			if (m_angleV < -DX_PI_F * kAngleVLowerMultiplier + kAngleVLimitOffset)
			{
				m_angleV = -DX_PI_F * kAngleVLowerMultiplier + kAngleVLimitOffset;
			}

		}

	}
}
