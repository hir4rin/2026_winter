#include "KingmechGatling.h"
#include "Character/Enemy/Kingmech/BossEnemyKingmech.h"
#include "Character/Player/Player.h"
#include "Main/System.h"
#include "Render/ModelShaderManager.h"
#include "Render/EffectManager.h"
#include "EffekseerForDXLib.h"
#include <algorithm>
#include <cmath>

namespace
{
	const char* kBossMeshFrameName = "polySurface789";//ボスのモデルのガトリングのメッシュのフレーム
	const char* kBossArmFrameName = "lowerarm_r";//ガトリングが付いている骨
	const char* kBossTargetFrameName = "spine_03";//投げる先
	const char* kHandFrameName = "hand_l";//プレイヤーの持たせる手(右手は刀)
	const VECTOR kArmOffset = VGet(-52.542f, -5.352f, 3.009f);//lowerarm_rからガトリングの真ん中まで//tools/KingmechGatling の extract_log

	const VECTOR kSpinAxis = VGet(0.0f, 1.0f, 0.0f);//回す軸(モデルの空間)

	//切り落とし
	constexpr float kCutJumpSpeed = 25.0f;//跳ね上がる速さ
	constexpr float kCutGravity = 1.2f;//重力
	constexpr float kCutSpinSpeed = 0.25f;//回る速さ(ラジアン/F)
	constexpr float kPopFrame = 20.0f;//跳ね上がってから手へ寄せ始めるまで
	constexpr float kToHandFrame = 25.0f;//手へ寄せる長さ

	//手で回す
	constexpr float kHoldSpinSpeed = 0.5f;

	//投げる
	constexpr float kThrowSpeed = 60.0f;//飛ぶ速さ
	constexpr float kThrowSpinSpeed = 0.7f;
	constexpr float kExplodeDistance = 225.0f;//ボスとこの距離まで近づいたら爆発
	constexpr float kStickDistance = 130.0f;//この距離まで近づいたら刺さる
	constexpr float kThrowMaxFrame = 90.0f;//これだけ飛んでも届かなければ爆発

	constexpr float kExplodeEffectScale = 45.0f;//爆発のエフェクトの大きさ

	//補給
	constexpr float kSupplyFrame = 30.0f;//落ちてくる長さ
	constexpr float kSupplyHeight = 2250.0f;//落ち始める高さ
	constexpr float kSupplyEffectScale = 12.0f;//付いたときの爆発の大きさ
}

KingmechGatling::KingmechGatling(std::weak_ptr<BossEnemyKingmech> owner) :
	m_owner(owner)
{
	//回転の値を初期化
	m_baseRot = MGetIdent();

	auto boss = m_owner.lock();
	if (!boss)return;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::KingmechGatlingModel));
	const int bossModel = boss->m_modelHandle;
	m_bossMeshIndex = MV1GetFrameMesh(bossModel, MV1SearchFrame(bossModel, kBossMeshFrameName), 0);
	m_bossArmFrame = MV1SearchFrame(bossModel, kBossArmFrameName);
	m_bossTargetFrame = MV1SearchFrame(bossModel, kBossTargetFrameName);
}

KingmechGatling::~KingmechGatling()
{
	MV1DeleteModel(m_modelHandle);
}

void KingmechGatling::Update()
{
	switch (m_phase)
	{
	case Phase::Cut:
		UpdateCut();
		break;
	case Phase::Hold:
		UpdateHold();
		break;
	case Phase::Thrown:
		UpdateThrown();
		break;
	case Phase::Supply:
		UpdateSupply();
		break;
	case Phase::Stuck:
		UpdateStuck();
		break;
	default:
		break;
	}
}

void KingmechGatling::Draw()
{
	Draw(MGetIdent());
}

void KingmechGatling::Draw(const MATRIX& extra)
{
	//補給中は回さず腕の真上に描く
	if (m_phase == Phase::Supply)
	{
		MV1SetMatrix(m_modelHandle, MMult(CalcSupplyMatrix(), extra));
		ModelShaderManager::GetInstance().DrawModel(m_modelHandle);
		return;
	}

	if (m_phase != Phase::Cut && m_phase != Phase::Hold && m_phase != Phase::Thrown && m_phase != Phase::Stuck)return;


	//回転//y軸
	MATRIX mat = MMult(MMult(MGetRotAxis(kSpinAxis, m_spinAngle), m_baseRot), MGetTranslate(m_pos.ToDxLibVector()));
	MV1SetMatrix(m_modelHandle, MMult(mat, extra));
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);
}

void KingmechGatling::Cut()
{
	if (m_phase != Phase::Attached)return;
	auto boss = m_owner.lock();
	if (!boss)return;

	//ボスのガトリングと同じ位置・向き・大きさから始める//ガトリングの回転の中心からまわす
	MATRIX mat = MMult(MGetTranslate(kArmOffset), MV1GetFrameLocalWorldMatrix(boss->m_modelHandle, m_bossArmFrame));
	m_pos = Vector3(mat.m[3][0], mat.m[3][1], mat.m[3][2]);
	m_baseRot = mat;
	m_baseRot.m[3][0] = m_baseRot.m[3][1] = m_baseRot.m[3][2] = 0.0f;

	//ボーンフレーム的に消せないからメッシュを消す
	MV1SetMeshVisible(boss->m_modelHandle, m_bossMeshIndex, FALSE);

	m_cutPos = m_pos;
	m_vel = Vector3(0.0f, kCutJumpSpeed, 0.0f);
	m_spinAngle = 0.0f;
	m_frame = 0.0f;
	m_phase = Phase::Cut;
}

void KingmechGatling::Throw()
{
	if (m_phase != Phase::Cut && m_phase != Phase::Hold)return;
	m_frame = 0.0f;
	m_phase = Phase::Thrown;
}

void KingmechGatling::StartSupply()
{
	if (m_phase != Phase::Gone)return;
	m_frame = 0.0f;
	m_phase = Phase::Supply;
}

void KingmechGatling::DebugRemove()
{
	if (m_phase != Phase::Attached)return;
	auto boss = m_owner.lock();
	if (!boss)return;
	MV1SetMeshVisible(boss->m_modelHandle, m_bossMeshIndex, FALSE);
	m_phase = Phase::Gone;
}

void KingmechGatling::ResetForFinisher()
{
	auto boss = m_owner.lock();
	if (!boss)return;
	MV1SetMeshVisible(boss->m_modelHandle, m_bossMeshIndex, FALSE);
	m_isStickThrow = false;
	m_phase = Phase::Gone;
}

void KingmechGatling::ThrowAndStick()
{
	Throw();
	m_isStickThrow = true;
}

void KingmechGatling::Vanish()
{
	m_phase = Phase::Gone;
}

Vector3 KingmechGatling::GetAttachedPos()const
{
	auto boss = m_owner.lock();
	if (!boss)return m_pos;
	const MATRIX mat = MMult(MGetTranslate(kArmOffset), MV1GetFrameLocalWorldMatrix(boss->m_modelHandle, m_bossArmFrame));
	return Vector3(mat.m[3][0], mat.m[3][1], mat.m[3][2]);
}

void KingmechGatling::UpdateCut()
{
	const float timeScale = System::GetInstance().GetTimeScale();
	m_frame += timeScale;
	m_spinAngle += kCutSpinSpeed * timeScale;

	//跳ね上がり
	m_vel.y -= kCutGravity * timeScale;
	m_cutPos += m_vel * timeScale;

	//少ししたら手へ寄せる
	float t = std::clamp((m_frame - kPopFrame) / kToHandFrame, 0.0f, 1.0f);
	t = t * t * (3.0f - 2.0f * t);
	m_pos = Vector3::Lerp(m_cutPos, GetHandPos(), t);
	if (t >= 1.0f)
	{
		m_frame = 0.0f;
		m_phase = Phase::Hold;
	}
}

void KingmechGatling::UpdateHold()
{
	m_spinAngle += kHoldSpinSpeed * System::GetInstance().GetTimeScale();
	//手の位置に保持
	m_pos = GetHandPos();
}

void KingmechGatling::UpdateThrown()
{
	const float timeScale = System::GetInstance().GetTimeScale();
	m_frame += timeScale;
	m_spinAngle += kThrowSpinSpeed * timeScale;

	//ボスへまっすぐ(ボスが動いても追いかける)
	Vector3 toTarget = GetTargetPos() - m_pos;
	if (m_isStickThrow)
	{
		if (toTarget.Magnitude() <= kStickDistance || m_frame >= kThrowMaxFrame)
		{
			StickToBoss();
			return;
		}
	}
	else if (toTarget.Magnitude() <= kExplodeDistance || m_frame >= kThrowMaxFrame)
	{
		Explode();
		return;
	}
	m_pos += toTarget.Normalize() * (kThrowSpeed * timeScale);
}

void KingmechGatling::Explode()
{
	//TODO: 専用の爆発エフェクト//今は仮でスキル3の着地の閃光
	int handle = PlayEffekseer3DEffect(System::GetInstance().GetHandle(AsyncData::GhostSkill3ImpactEffect));
	SetPosPlayingEffekseer3DEffect(handle, m_pos.x, m_pos.y, m_pos.z);
	SetScalePlayingEffekseer3DEffect(handle, kExplodeEffectScale, kExplodeEffectScale, kExplodeEffectScale);

	m_phase = Phase::Gone;

	//ボスにダメージ
	auto boss = m_owner.lock();
	if (boss)boss->OnGatlingExploded();
}

void KingmechGatling::StickToBoss()
{
	const Vector3 target = GetTargetPos();
	const Vector3 toOut = (m_pos - target).Normalize();
	m_stuckOffset = toOut * kStickDistance;
	m_pos = target + m_stuckOffset;

	//モデルの-Xが銃口
	const VECTOR axisX = toOut.ToDxLibVector();
	const VECTOR axisZ = VNorm(VCross(axisX, VGet(0.0f, 1.0f, 0.0f)));
	const VECTOR axisY = VCross(axisZ, axisX);
	const float scale = VSize(VGet(m_baseRot.m[0][0], m_baseRot.m[0][1], m_baseRot.m[0][2]));
	m_baseRot = MGetIdent();
	const VECTOR axes[3] = { axisX, axisY, axisZ };
	for (int i = 0; i < 3; ++i)
	{
		m_baseRot.m[i][0] = axes[i].x * scale;
		m_baseRot.m[i][1] = axes[i].y * scale;
		m_baseRot.m[i][2] = axes[i].z * scale;
	}
	m_spinAngle = 0.0f;
	m_phase = Phase::Stuck;
}

void KingmechGatling::UpdateStuck()
{
	m_pos = GetTargetPos() + m_stuckOffset;
}

void KingmechGatling::UpdateSupply()
{
	m_frame += System::GetInstance().GetTimeScale();
	if (m_frame >= kSupplyFrame)
	{
		FinishSupply();
	}
}

void KingmechGatling::FinishSupply()
{
	auto boss = m_owner.lock();
	if (!boss)return;

	const MATRIX mat = CalcSupplyMatrix();
	const Vector3 pos(mat.m[3][0], mat.m[3][1], mat.m[3][2]);
	EffectManager::GetInstance().Play(AsyncData::KingmechMissileExplodeEffect, pos, 0.0f, kSupplyEffectScale);

	//本体のメッシュに切り替える
	MV1SetMeshVisible(boss->m_modelHandle, m_bossMeshIndex, TRUE);
	m_phase = Phase::Attached;
}

MATRIX KingmechGatling::CalcSupplyMatrix()const
{
	auto boss = m_owner.lock();
	if (!boss)return MGetIdent();

	MATRIX mat = MMult(MGetTranslate(kArmOffset), MV1GetFrameLocalWorldMatrix(boss->m_modelHandle, m_bossArmFrame));
	const float t = std::clamp(m_frame / kSupplyFrame, 0.0f, 1.0f);
	mat.m[3][1] += kSupplyHeight * (1.0f - t * t);//だんだん速く
	return mat;
}



Vector3 KingmechGatling::GetHandPos()const
{
	auto boss = m_owner.lock();
	if (!boss)return m_pos;
	auto player = boss->m_player.lock();
	if (!player)return m_pos;
	int model = player->GetModelHandle();
	return Vector3(MV1GetFramePosition(model, MV1SearchFrame(model, kHandFrameName)));
}

Vector3 KingmechGatling::GetTargetPos()const
{
	auto boss = m_owner.lock();
	if (!boss)return m_pos;
	return Vector3(MV1GetFramePosition(boss->m_modelHandle, m_bossTargetFrame));
}
