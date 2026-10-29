#include "KingmechMissile.h"
#include "Main/System.h"
#include "Render/EffectManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	constexpr float kWarningTime = 45.0f;//予告の円の長さ
	constexpr float kFallTime = 15.0f;//落ちてくる長さ//エフェクトも15Fで地面に届く
	constexpr float kExplodeTime = 4.0f;//爆発の長さ
	constexpr float kEffectScale = 20.0f;//エフェクトは1/20で作っている//20倍で半径約200
}

KingmechMissile::KingmechMissile(const Vector3& landPos) :
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
		break;
	default:
		break;
	}
}
