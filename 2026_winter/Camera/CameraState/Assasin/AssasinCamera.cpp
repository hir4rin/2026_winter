#include "AssasinCamera.h"
#include "../../CameraManager.h"
#include "Player.h"
#include "../PlayerFollowCamera.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../Math/Matrix4x4.h"
#include "../System.h"
#include <algorithm>
#include <cmath>
namespace
{
	constexpr float kBlendDuration = 60.0f;//ブレンドにかけるフレーム数
	constexpr float kBlendEasingPower = 0.5f;//ブレンドのイージング指数
	constexpr float kStayFrame = 20.0f;//距離を変え始めるまでのフレーム数
	constexpr float kStartDistance = 350.0f;//プレイヤーからカメラまでの水平距離(開始)
	constexpr float kEndDistance = 200.0f;//プレイヤーからカメラまでの水平距離(終了)
	constexpr float kDistanceChangeFrame = 40.0f;//距離を変えるフレーム数
	constexpr float kDistanceEasingPower = 2.0f;//距離変化のイージング指数
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//カメラの高さ(Vector3)
}

AssasinCameraState::AssasinCameraState(std::weak_ptr<CameraManager> owner) : CameraStateBase(owner)
{
}

AssasinCameraState::~AssasinCameraState()
{
}

void AssasinCameraState::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = data.angleV;
	m_pos = data.pos;
	m_target = data.target;
	m_goalPos = data.pos;
	m_goalTarget = data.target;
	m_distance = kStartDistance;
	ResetBlend(data.pos, data.target);
	////前のStateが上書きブレンドを指定していたらそれを使う(AssasinCameraStartからのSlerp)
	if (data.overrideSetting.mode != BlendSetting::Mode::None)
	{
		m_activeBlend = data.overrideSetting;
	}
}

void AssasinCameraState::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	//TODO:暗殺演出のカメラ位置・注視点をここで計算する
	FixCameraPos();

	if (IsBlending())
	{
		UpdateBlend(m_goalPos, m_goalTarget);
	}
	else
	{
		m_pos = m_goalPos;
		m_target = m_goalTarget;
	}

	float timeScale = System::GetInstance().GetTimeScale();
	if (m_timer <= kStayFrame)
	{
		m_timer += 1.0f * timeScale;
		return;
	}

	//タイマーが終わったら、距離をイージングで縮める
	m_distanceTimer += 1.0f * timeScale;
	float t = std::clamp(m_distanceTimer / kDistanceChangeFrame, 0.0f, 1.0f);
	m_distance = std::lerp(kStartDistance, kEndDistance, Easing::Apply(EasingMode::EaseInOut, t, kDistanceEasingPower));

	//距離の変化が終わったら次のStateへ
	if (t >= 1.0f)
	{
		cameraManager->ChangeState(std::make_shared<PlayerFollowCamera>(m_owner));
		return;
	}

}

void AssasinCameraState::Exit()
{
	//m_angleH/m_angleVを、カメラ→注視点のベクトルから計算し直す(次のStateに正しい向きを渡すため)
	Vector3 toTarget = m_target - m_pos;
	float horizontalDist = Vector3(toTarget.x, 0.0f, toTarget.z).Magnitude();
	m_angleH = atan2f(toTarget.x, toTarget.z);
	m_angleV = atan2f(toTarget.y, horizontalDist);
}

void AssasinCameraState::FixCameraPos()
{
	//TODO:m_goalPos / m_goalTarget を算出する
	//プレイヤーと敵の距離近め
	//カメラ->テキー＞プレイヤーの順で、下から上の構図
	//三分割法意識

	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;
	auto enemy = player->GetAssasinTarget();
	if (!enemy)return;

	//PtoEVecを15度ずらす
	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();

	Vector3 PtoEVec = (enemyPos - playerPos).Normalize();
	PtoEVec.y = 0.0f;
	PtoEVec = PtoEVec * m_distance;

	float angle = DX_PI_F / 12;

	//水平方向の回転//敵との距離によってこの角度を帰る
	auto rotY = Matrix4x4::MakeRotationY(angle);

	//DxLibに変換
	auto EtoPVecDx = PtoEVec.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する

	auto RotPtoC = VTransform(EtoPVecDx, rotYMat);//回転させる
	RotPtoC = VAdd(RotPtoC, kCameraHeight.ToDxLibVector());


	auto pos = VAdd(RotPtoC, player->GetRigidBody().GetPos().ToDxLibVector());//プレイヤーの座標に足す
	m_goalPos = Vector3::FromDxLibVector(pos);

	//注視点
	//プレイヤーと敵の腰ボーンの中間地点を見る
	m_goalTarget = (player->GetWaistPos() + enemy->GetWaistPos()) * 0.5f;
	

}

void AssasinCameraState::CameraSetting()
{
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting AssasinCameraState::GetBlendSetting() const
{
	return BlendSetting{
		.mode = BlendSetting::Mode::Lerp,
		.duration = kBlendDuration,
		.easingMode = EasingMode::EaseOutBack,
		.easingPower = kBlendEasingPower
	};
}

BlendSetting AssasinCameraState::GetOverrideBlendSetting()const
{
	//return BlendSetting{
	//	.mode = BlendSetting::Mode::Lerp,
	//	.duration = 20.0f,
	//	.easingMode = EasingMode::EaseInOut,
	//	.easingPower = kBlendEasingPower
	//};

	return BlendSetting{
		.mode = BlendSetting::Mode::None,
		.duration = 20.0f,
		.easingMode = EasingMode::EaseInOut,
		.easingPower = kBlendEasingPower
	};
}
