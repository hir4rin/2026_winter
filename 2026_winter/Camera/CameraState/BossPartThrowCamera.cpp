#include "BossPartThrowCamera.h"
#include "../CameraManager.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"
#include <cmath>

namespace
{
	const Vector3 kCameraHeight = Vector3(0.0f, 150.0f, 0.0f);//注視点の高さ//PlayerFollowCameraと同じ

	constexpr float kToPlayerLength = 350.0f;//注視点からカメラまでの距離//PlayerFollowCameraと同じ
	constexpr float kAngleV = DX_PI_F / 10.0f + DX_PI_F / 18.0f;//垂直角度//PlayerFollowCameraの初期値+5度
	constexpr float kRotateAngle = DX_PI_F / 3.0f;//敵→プレイヤーの軸から回す角度//BattleSmoothCameraと同じ

	constexpr float kBlendDuration = 30.0f;//ブレンドのフレーム数
	constexpr float kBlendEasingPower = 2.0f;//ブレンドのイージング指数
}

BossPartThrowCamera::BossPartThrowCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

BossPartThrowCamera::~BossPartThrowCamera()
{
}

void BossPartThrowCamera::Enter(CameraData data)
{
	m_angleH = data.angleH;
	m_angleV = kAngleV;
	DecideAngleH(data.pos);
	ResetBlend(data.pos, data.target);
}

void BossPartThrowCamera::Update()
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!player)return;

	m_goalTarget = player->GetCameraFocusPos() + kCameraHeight;
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

void BossPartThrowCamera::Exit()
{
}

void BossPartThrowCamera::FixCameraPos()
{
	//m_angleHはカメラ→注視点の向き
	Vector3 toCamera = Vector3(-sinf(m_angleH) * cosf(m_angleV), sinf(m_angleV), -cosf(m_angleH) * cosf(m_angleV));
	m_goalPos = m_goalTarget + toCamera * kToPlayerLength;
}

void BossPartThrowCamera::DecideAngleH(const Vector3& cameraPos)
{
	auto cameraManager = m_owner.lock();
	if (!cameraManager)return;
	auto enemy = cameraManager->GetAttackTarget();
	auto player = cameraManager->GetContext()->m_player.lock();
	if (!enemy)return;
	if (!player)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = enemy->GetRigidBody().GetPos();
	Vector3 flatCameraPos = cameraPos;
	//y座標を0にする
	playerPos.y = enemyPos.y = flatCameraPos.y = 0.0f;

	Vector3 toEnemy = enemyPos - playerPos;
	if (toEnemy.Magnitude() <= 0.0f)return;

	//カメラに近いほうへ回す//BattleSmoothCameraと同じ判定
	Vector3 EtoPVec = (playerPos - enemyPos).Normalize();
	Vector3 upVec = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 rotateBase = EtoPVec.Cross(upVec).Normalize();
	Vector3 PtoCVec = (flatCameraPos - playerPos).Normalize();
	float rotateAngle = (rotateBase.Dot(PtoCVec) >= 0.0f) ? -kRotateAngle : kRotateAngle;
	rotateAngle = -kRotateAngle;

	m_angleH = atan2f(toEnemy.x, toEnemy.z) + rotateAngle;
}

using BlendSetting = CameraStateBase::BlendSetting;
BlendSetting BossPartThrowCamera::GetBlendSetting() const
{
	//プレイヤーを中心に回り込む
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
