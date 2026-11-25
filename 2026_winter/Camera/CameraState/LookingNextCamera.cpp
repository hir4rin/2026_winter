#include "LookingNextCamera.h"
#include "../CameraManager.h"
#include "PlayerFollowCamera.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/System.h"
#include "../../Math/Matrix4x4.h"
#include <cmath>


namespace
{
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//カメラの高さ(Vector3)//PlayerFollowCameraと同じ値

	constexpr float kToPlayerLength = 50.0f;//プレイヤーからカメラまでの距離
	constexpr float kAngleV = 5.0f * DX_PI_F / 180.0f;//垂直角度
	constexpr float kLookDistance = 300.0f;//プレイヤー→チェックポイント方向に注視点をずらす距離

	constexpr float kCameraNear = 200.0f;//ニアクリップ
	constexpr float kCameraFar = 5000.0f;//ファークリップ
	constexpr float kDefaultCameraNear = 100.0f;//SceneMainと同じ値
	constexpr float kDefaultCameraFar = 5000.0f;//SceneMainと同じ値

	constexpr float kBlendDuration = 15.0f;//ブレンドにかけるフレーム数
	constexpr float kBlendEasingPower = 2.0f;//ブレンドのイージング指数
}

LookingNextCamera::LookingNextCamera(std::weak_ptr<CameraManager> owner):CameraStateBase(owner)
{
}

LookingNextCamera::~LookingNextCamera()
{
}

void LookingNextCamera::Enter(CameraData data)
{
	m_angleV = data.angleV;
	ResetBlend(data.pos, data.target);

	SetCameraNearFar(kCameraNear, kCameraFar);

	//歩くとカメラ基準で向きが変わるので、向きは入ったときだけ決める
	m_angleH = data.angleH;
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//プレイヤーの向いている方向の後ろにカメラを置く
	Vector3 forward = player->GetTargetVec();
	forward.y = 0.0f;
	if (forward.Magnitude() > 0.0f)
	{
		m_angleH = atan2f(forward.x, forward.z);
	}

	//タイムスケールを遅くする
	System::GetInstance().SetTimeScale(0.1f);
}

void LookingNextCamera::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//RTを離した・次のチェックポイントが無くなったら、PlayerCameraに戻す
	Vector3 checkpointPos;
	if (!Input::GetInstance().IsPressed("RT") || !cameraManager->GetNextCheckpointPos(checkpointPos))
	{
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}

	//目標座標を計算
	//注視点//プレイヤーの頭からチェックポイントのほうへ固定距離ずらす
	Vector3 headPos = player->GetRigidBody().GetPos() + kCameraHeight;
	Vector3 pToCVec = checkpointPos - headPos;
	if (pToCVec.Magnitude() > 0.0f) m_goalTarget = headPos + pToCVec;
	FixCameraPos();

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

void LookingNextCamera::Exit()
{
	//注視点が遠いと次のカメラのブレンドが変になるので、向きはそのままで近くに寄せる
	Vector3 toTarget = m_target - m_pos;
	if (toTarget.Magnitude() > 0.0f)
	{
		m_target = m_pos + toTarget.Normalize() * kLookDistance;
		toTarget = m_target - m_pos;
	}

	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(-toTarget.y, horizontalDist);//プラスでカメラが上(FixCameraPosと同じ向き)

	//ファーが短いままだと遠くが消えるので元に戻す
	SetCameraNearFar(kDefaultCameraNear, kDefaultCameraFar);


	//タイムスケールを元に戻す
	System::GetInstance().SetTimeScale(1.0f);
}

void LookingNextCamera::FixCameraPos()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	//PlayerFollowCamera::FixCameraPosと同じ計算//プレイヤーの頭の位置から後ろへ
	auto rotY = Matrix4x4::MakeRotationY(m_angleH);
	auto rotX = Matrix4x4::MakeRotationX(kAngleV);

	auto CtoP = Vector3(0.0f, 0.0f, -kToPlayerLength);//プレイヤーからカメラへのベクトル
	//DxLibに変換
	auto CtoPVec = CtoP.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する
	auto rotXMat = Matrix4x4::ToDxLibMatrix(rotX);//回転行列を転置する
	auto RotCtoP = VTransform(CtoPVec, rotXMat);//回転させる
	RotCtoP = VTransform(RotCtoP, rotYMat);//回転させる

	Vector3 headPos = player->GetRigidBody().GetPos() + kCameraHeight;
	auto pos = VAdd(RotCtoP, headPos.ToDxLibVector());//プレイヤーの頭の位置に足す
	m_goalPos = Vector3::FromDxLibVector(pos);
}

void LookingNextCamera::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting LookingNextCamera::GetBlendSetting() const
{
	auto cameraManager = m_owner.lock();
	std::shared_ptr<Player> player;
	if (cameraManager) player = cameraManager->GetContext()->m_player.lock();

	return BlendSetting{
	.mode = BlendSetting::Mode::Slerp,
	.duration = kBlendDuration,
	.easingMode = EasingMode::EaseOut,
	.easingPower = kBlendEasingPower,
	.pivot = (player) ? player->GetRigidBody().GetPos() : Vector3(),
	};
}
