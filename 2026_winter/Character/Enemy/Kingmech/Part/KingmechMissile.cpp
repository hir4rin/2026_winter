#include "KingmechMissile.h"
#include "Main/System.h"
#include "Render/EffectManager.h"
#include "EffekseerForDXLib.h"
#include "Collision/FixedAttackCol.h"
#include "Collision/SphereShape.h"

namespace
{
	constexpr float kWarningTime = 45.0f;//予告の円の長さ
	constexpr float kFallTime = 15.0f;//落ちてくる長さ//エフェクトも15Fで地面に届く
	constexpr float kExplodeTime = 4.0f;//爆発の長さ//判定もこの長さだけ出す
	constexpr float kEffectScale = 20.0f;//エフェクトは1/20で作っている//20倍で半径約200

	constexpr float kExplodeRadius = 190.0f;//爆発の判定の半径//予告の円に合わせる//中心は地面
	constexpr float kExplodePower = 30.0f;//爆発の攻撃力
}

KingmechMissile::KingmechMissile(std::weak_ptr<CharacterBase> owner, const Vector3& landPos) :
	m_owner(owner),
	m_landPos(landPos)
{
	m_warningEffect = EffectManager::GetInstance().Play(AsyncData::KingmechMissileWarningEffect, m_landPos, 0.0f, kEffectScale);
}

KingmechMissile::~KingmechMissile()
{
	StopEffekseer3DEffect(m_warningEffect);
}

void KingmechMissile::Update()
{
	m_timer += System::GetInstance().GetTimeScale();

	switch (m_phase)
	{
	case Phase::Warning:
		if (m_timer >= kWarningTime)ChangePhase(Phase::Falling);
		break;
	case Phase::Falling:
		if (m_timer >= kFallTime)ChangePhase(Phase::Explode);
		break;
	case Phase::Explode:
		if (m_timer >= kExplodeTime)ChangePhase(Phase::Finished);
		break;
	default:
		break;
	}
}

void KingmechMissile::ChangePhase(Phase next)
{
	m_phase = next;
	m_timer = 0.0f;

	switch (m_phase)
	{
	case Phase::Falling:
		EffectManager::GetInstance().Play(AsyncData::KingmechMissileFallEffect, m_landPos, 0.0f, kEffectScale);
		break;
	case Phase::Explode:
		//着弾したら予告の円は消す
		StopEffekseer3DEffect(m_warningEffect);
		m_warningEffect = -1;
		EffectManager::GetInstance().Play(AsyncData::KingmechMissileExplodeEffect, m_landPos, 0.0f, kEffectScale);
		CreateExplodeCol();
		break;
	default:
		break;
	}
}

void KingmechMissile::CreateExplodeCol()
{
	CharacterBase::AttackData data = {
		.attackPower = kExplodePower,
		.brokenRate = 0.0f,
		.knockBackPower = Vector3(20.0f, 20.0f, 0.0f),
		.knockBackFrame = 0.0f,
		.hitStopTime = 0.1f,
		.kAttackColOffset = 0.0f,
		.isKirimomi = true
	};
	//プレイヤーの着地の衝撃と同じ作り//寿命で消える
	m_explodeCol = std::make_shared<FixedAttackCol>(m_owner, data, m_landPos);
	m_explodeCol->ColInit({
		.pos = m_landPos,
		.offset = Vector3(0.0f, 0.0f, 0.0f),
		.shape = std::make_unique<SphereShape>(kExplodeRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::Attack},
		.isActive = true,
		.isTrigger = true,
		.lifeTime = kExplodeTime
		});
}
