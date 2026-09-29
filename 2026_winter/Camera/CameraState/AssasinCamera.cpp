#include "AssasinCamera.h"
#include "../CameraManager.h"
#include "Player.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../../Math/Matrix4x4.h"
namespace
{
	constexpr float kBlendDuration = 15.0f;//ブレンドにかけるフレーム数
	constexpr float kBlendEasingPower = 0.5f;//ブレンドのイージング指数
	constexpr float kEtoPVecLength = 200.0f;
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
	ResetBlend(data.pos, data.target);
}

void AssasinCameraState::Update()
{
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
	PtoEVec = PtoEVec * kEtoPVecLength;

	float angle = 15.0f;

	//水平方向の回転//敵との距離によってこの角度を帰る
	auto rotY = Matrix4x4::MakeRotationY(angle);

	//DxLibに変換
	auto EtoPVecDx = PtoEVec.ToDxLibVector();
	auto rotYMat = Matrix4x4::ToDxLibMatrix(rotY);//回転行列を転置する


	//注視点
	//一旦Player
	m_goalTarget = playerPos;

	//


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
		.easingPower = kBlendEasingPower
	};
}
