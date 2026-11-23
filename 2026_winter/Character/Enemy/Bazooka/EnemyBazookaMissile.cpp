#include "EnemyBazookaMissile.h"
#include "EnemyBazookaMissileCol.h"
#include "Main/System.h"
#include "Render/EffectManager.h"
#include "EffekseerForDXLib.h"
#include "Collision/FixedAttackCol.h"
#include "Collision/SphereShape.h"
#include "DxLib.h"
#include <cmath>
#include <algorithm>

namespace
{
	constexpr float kSpeed = 30.0f;//1Fで進む距離

	//sinの揺れ//幅は距離に比例させて、上限で止める
	constexpr float kSideWidthRate = 0.2f;//横の幅(距離に対する割合)
	constexpr float kMaxSideWidth = 400.0f;//横の幅の上限
	constexpr float kUpWidthRate = 0.15f;//縦の幅(距離に対する割合)
	constexpr float kMaxUpWidth = 300.0f;//縦の幅の上限
	constexpr float kSideWaveNum = 1.0f;//横の山の数//整数にすると着弾点にぴったり落ちる
	constexpr float kUpWaveNum = 1.0f;//縦の山の数//1なら上に弧を描く
	constexpr float kMinWidthRate = 0.7f;//幅の乱数の下限(1.0までのどこか)

	constexpr float kFlyColRadius = 40.0f;//飛んでいる間の判定の半径//1Fで進む距離より大きくしてすり抜けを防ぐ

	//予告・爆発はKingmechのミサイルのエフェクトを使う
	constexpr float kWarningTime = 45.0f;//予告のエフェクトの長さ
	constexpr float kEffectScale = 20.0f;//エフェクトは1/20で作っている
	constexpr float kExplodeTime = 4.0f;//爆発の長さ//判定もこの長さだけ出す
	constexpr float kExplodeRadius = 190.0f;//爆発の判定の半径//予告の円に合わせる
	constexpr float kExplodePower = 30.0f;//爆発の攻撃力
}

EnemyBazookaMissile::EnemyBazookaMissile(std::weak_ptr<CharacterBase> owner, const Vector3& startPos, const Vector3& targetPos)
{
	m_owner = owner;
	m_startPos = startPos;
	m_targetPos = targetPos;
	m_pos = startPos;
	m_prevPos = startPos;

	m_distance = (m_targetPos - m_startPos).Magnitude();

	//進む向き
	Vector3 dir = (m_targetPos - m_startPos).Normalize();

	//進む向きに直角な横と上
	m_right = Vector3(0, 1, 0).Cross(dir).Normalize();
	m_up = dir.Cross(m_right).Normalize();

	m_sideWidth = (std::min)(m_distance * kSideWidthRate, kMaxSideWidth);//距離の2割か上限の小さい方
	m_upWidth = (std::min)(m_distance * kUpWidthRate, kMaxUpWidth);//距離の1.5割か上限の小さい方

	// 1発ごとに変える
	float rate = kMinWidthRate + (1.0f - kMinWidthRate) * GetRand(1000) / 1000.0f;   // 0.7〜1.0
	m_sideWidth *= rate;
	m_upWidth *= rate;
	if (GetRand(1) == 0) m_sideWidth *= -1.0f;   // 半分の確率で左に曲げる
}

EnemyBazookaMissile::~EnemyBazookaMissile()
{
	//途中で消されたときに予告の円が残らないように
	if(m_warningEffect >= 0)
	{
		StopEffekseer3DEffect(m_warningEffect);
	}
	//煙は消えるまで残す
	if (m_flyEffect >= 0) GetEffekseer3DManager()->StopRoot(m_flyEffect);
}

void EnemyBazookaMissile::Init()
{
	m_warningEffect = EffectManager::GetInstance().Play(AsyncData::KingmechMissileWarningEffect, m_targetPos, 0.0f, kEffectScale);

	//ミサイル着弾までの割合に調整
	SetSpeedPlayingEffekseer3DEffect(m_warningEffect, kWarningTime / (m_distance / kSpeed));

	//弾のエフェクト
	m_flyEffect = EffectManager::GetInstance().Play(AsyncData::EnemyBazookaMissileEffect, m_pos, 0.0f, kEffectScale);
	UpdateFlyEffect();

	//飛んでいる間の判定//ダメージは爆発だけ
	m_flyCol = std::make_shared<EnemyBazookaMissileCol>();
	m_flyCol->ColInit({ .pos = m_pos, .offset = Vector3(0,0,0),
		.shape = std::make_unique<SphereShape>(kFlyColRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::None},
		.isActive = true, .isTrigger = true });
	m_flyCol->SetPos(m_pos);
}

void EnemyBazookaMissile::Update()
{
	switch (m_phase)
	{
		case Phase::Fly:
			UpdateFly();
			break;
		case Phase::Explode:
			m_timer += System::GetInstance().GetTimeScale();
			if (m_timer >= kExplodeTime)
			{
				m_phase = Phase::Finished;
			}
			break;
		case Phase::Finished:
			break;
	}

}

void EnemyBazookaMissile::UpdateFlyEffect()
{
	if (m_flyEffect < 0)return;

	Vector3 dir = m_pos - m_prevPos;
	//dirの長さがほぼ0なら、最初の向きを使う
	if(dir.Magnitude() < 0.001f)
	{
		dir = m_targetPos - m_startPos;
	}
	if (dir.Magnitude() < 0.001f)dir = Vector3(0, 0, 1);
	dir = dir.Normalize();

	//エフェクトは-Zが先端
	float yaw = atan2f(dir.x, dir.z);
	float pitch = -asinf((std::clamp)(dir.y, -1.0f, 1.0f));
	SetPosPlayingEffekseer3DEffect(m_flyEffect, m_pos.x, m_pos.y, m_pos.z);
	SetRotationPlayingEffekseer3DEffect(m_flyEffect, pitch, yaw, 0.0f);
}

void EnemyBazookaMissile::UpdateFly()
{
	//前のフレームでプレイヤーか物に触れていたら爆発
	if(m_flyCol && m_flyCol->IsTouched())
	{
		Explode(m_pos);
		return;
	}

	m_rate += kSpeed / m_distance * System::GetInstance().GetTimeScale();//1Fで進む割合
	if (m_rate > 1.0f) m_rate = 1.0f;
	if (m_distance == 0.0f) m_rate = 1.0f;
	m_prevPos = m_pos;

	m_pos = CalcFlyPos(m_rate);

	//前の位置から今の位置の線でステージに当たったら爆発
	MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(m_flyCol->GetStageHandle(), -1, m_prevPos.ToDxLibVector(), m_pos.ToDxLibVector());
	if(hit.HitFlag)
	{
		Explode(Vector3::FromDxLibVector(hit.HitPosition));
		return;
	}

	//着弾点に着いたら爆発
	if(m_rate >= 1.0f)
	{
		Explode(m_targetPos);
		return;
	}

	if (m_flyCol) m_flyCol->SetPos(m_pos);
	UpdateFlyEffect();
}

Vector3 EnemyBazookaMissile::CalcFlyPos(float rate) const
{
	Vector3 pos = Vector3::Lerp(m_startPos, m_targetPos, rate);

	//横と縦にずらす//rateが0と1でずれが0
	pos += m_right * (m_sideWidth * sinf(DX_PI_F * rate * kSideWaveNum));
	pos += m_up    * (m_upWidth   * sinf(DX_PI_F * rate * kUpWaveNum));

	return pos;
}

void EnemyBazookaMissile::Explode(const Vector3& pos)
{
	m_phase = Phase::Explode;
	m_timer = 0.0f;
	m_pos = pos;
	//予告の円を止める
	StopEffekseer3DEffect(m_warningEffect);
	m_warningEffect = -1;

	//弾を消す//煙は消えるまで残す
	if (m_flyEffect >= 0) GetEffekseer3DManager()->StopRoot(m_flyEffect);
	m_flyEffect = -1;

	//飛んでいる間の判定を消す
	m_flyCol.reset();

	EffectManager::GetInstance().Play(AsyncData::KingmechMissileExplodeEffect, m_pos, 0.0f, kEffectScale);
	CreateExplodeCol();

}

void EnemyBazookaMissile::CreateExplodeCol()
{
	CharacterBase::AttackData data = {
		.attackPower = kExplodePower,
		.brokenRate = 0.0f,
		.knockBackPower = Vector3(20.0f, 20.0f, 0.0f),
		.knockBackFrame = 0.0f,
		.hitStopTime = 0.1f,
		.hitStopFrame = 3.0f,
		.kAttackColOffset = 0.0f,
		.isKirimomi = true
	};

	m_attackCol = std::make_shared<FixedAttackCol>(m_owner, data, m_pos);
	m_attackCol->ColInit({
		.pos = m_pos,
		.offset = Vector3(0, 0, 0),
		.shape = std::make_unique<SphereShape>(kExplodeRadius),
		.tag = { Collider::Faction::Enemy, Collider::ColRole::Attack },
		.isActive = true,
		.isTrigger = true,
		.lifeTime = kExplodeTime
	});

}
