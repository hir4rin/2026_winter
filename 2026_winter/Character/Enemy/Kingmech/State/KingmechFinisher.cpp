#include "KingmechFinisher.h"
#include "Character/Enemy/Kingmech/BossEnemyKingmech.h"
#include "Character/Enemy/Kingmech/Part/KingmechGatling.h"
#include "Character/Enemy/Kingmech/Part/KingmechMissile.h"
#include "Character/Player/Player.h"
#include "Camera/CameraManager.h"
#include "Camera/CameraState/BossFinisherCamera.h"
#include "Battle/BattleManager.h"
#include "Render/EffectManager.h"
#include "Math/Easing.h"
#include "Main/System.h"
#include "Collision/HitCol.h"
#include <algorithm>
#include <cmath>

namespace
{
	const std::string kKneelStartAnim = "KnockStart";
	const std::string kKneelLoopAnim = "KnockLoop";
	const std::string kRoarAnim = "Roar";

	//フェーズの長さ(実フレーム)
	constexpr float kStaggerTime = 45.0f;
	constexpr float kWhiteInTime = 12.0f;
	constexpr float kWhiteOutTime = 14.0f;
	constexpr float kSupplyStartFrame = 12.0f;
	constexpr float kSupplyTime = 80.0f;
	constexpr float kLeapFrame = 20.0f;
	constexpr float kCutFrame = 20.0f;//切り落とすフレーム
	constexpr float kCutTime = 68.0f;
	constexpr float kSilenceTime = 36.0f;
	constexpr float kSlashHitFrame = 8.0f;//刃が胸を通るフレーム
	constexpr float kSlashMoveFrame = 16.0f;
	constexpr float kSlashLineFrame = 22.0f;
	constexpr float kSlashTime = 30.0f;
	constexpr float kSheathFrame = 34.0f;//納刀のフレーム
	constexpr float kSheathTime = 58.0f;
	constexpr float kSplitTime = 48.0f;
	constexpr float kExplodeFadeTime = 50.0f;
	constexpr float kExplodeTime = 85.0f;

	constexpr float kStaggerTimeScale = 0.35f;
	constexpr float kSilenceTimeScale = 0.2f;
	constexpr int kUltFrame = 100000;//SetUltEndで終わらせる

	//配置(闘技場の中央からの距離)
	constexpr float kPlayerStartFront = 900.0f;
	constexpr float kPlayerEndBack = 650.0f;
	constexpr float kAirFrontOffset = 150.0f;//腕のどれだけ前で止まるか
	constexpr float kThrowFrame = 17.2f;//PlayerStateBossPartThrowと同じ

	//斬る面
	constexpr float kCutTiltAngle = 30.0f * DX_PI_F / 180.0f;
	constexpr float kSlideLength = 240.0f;//上半身がずれる距離
	constexpr float kTiltMaxAngle = 12.0f * DX_PI_F / 180.0f;//上半身が傾く角度
	constexpr float kBloodSideOffset = 170.0f;

	//エフェクト
	constexpr float kCutEffectScale = 10.0f;
	constexpr float kBloodScale = 55.0f;
	constexpr float kExplodeScale = 50.0f;
	constexpr float kExplodeSubScale = 32.0f;
	constexpr float kExplodeSubOffset = 260.0f;
	constexpr float kBloodOwnSpeed = 0.65f;

	//カメラ(手で詰める値はここ)
	constexpr float kFovWide = 58.0f;
	constexpr float kFovNormal = 50.0f;
	constexpr float kFovClose = 38.0f;
}

KingmechFinisher::KingmechFinisher(std::weak_ptr<EnemyBase> owner) :
	EnemyStateBase(owner)
{
}

KingmechFinisher::~KingmechFinisher()
{
}

void KingmechFinisher::Enter()
{
	auto owner = GetBoss();
	if (!owner)return;
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	owner->m_isLookAtPlayer = false;

	auto battle = System::GetInstance().GetBattleMgr();
	battle->GetFinisherScreen().isPlaying = true;
	battle->SetIsEventPlaying(true);

	m_center = owner->m_arenaCenter;
	Vector3 toPlayer = owner->TargetPlayerPos() - m_center;
	toPlayer.y = 0.0f;
	m_front = (toPlayer.Magnitude() > 0.0f) ? toPlayer.Normalize() : owner->m_targetVec;
	m_side = Vector3(0.0f, 1.0f, 0.0f).Cross(m_front).Normalize();

	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kKneelStartAnim), false, owner->GetAnimSpeed(kKneelStartAnim));
	System::GetInstance().SetTimeScaleForFrames(kStaggerTimeScale, static_cast<int>(kStaggerTime));
}

void KingmechFinisher::Update()
{
	auto owner = GetBoss();
	if (!owner)return;
	auto battle = System::GetInstance().GetBattleMgr();
	auto& screen = battle->GetFinisherScreen();
	auto playerState = GetPlayerState();
	m_frame += 1.0f;

	//途中でプレイヤーが抜けたら終わる
	if (m_phase != Phase::Stagger && m_phase != Phase::Done && !playerState)
	{
		Finish();
		return;
	}

	switch (m_phase)
	{
	case Phase::Stagger:
		UpdateKneel();
		screen.whiteRate = std::clamp((m_frame - (kStaggerTime - kWhiteInTime)) / kWhiteInTime, 0.0f, 1.0f);
		if (m_frame >= kStaggerTime)
		{
			//プレイヤーが同時にやられたときは普通に倒れる
			if (!playerState)
			{
				Cleanup();
				owner->ChangeState(std::make_shared<EnemyDie>(owner));
				return;
			}
			SetupStage();
			ChangePhase(Phase::Supply);
		}
		break;

	case Phase::Supply:
		screen.whiteRate = std::clamp(1.0f - m_frame / kWhiteOutTime, 0.0f, 1.0f);
		if (!m_isEventDone && m_frame >= kSupplyStartFrame)
		{
			m_isEventDone = true;
			owner->m_gatling->StartSupply();
		}
		if (m_isEventDone && !m_isSecondEventDone && owner->m_gatling->GetPhase() == KingmechGatling::Phase::Attached)
		{
			m_isSecondEventDone = true;
			Shake(10.0f, 18.0f);
		}
		if (m_frame >= kSupplyTime)ChangePhase(Phase::Cut);
		break;

	case Phase::Cut:
		UpdateKneel();
		if (!m_isEventDone && m_frame >= kCutFrame)
		{
			m_isEventDone = true;
			const Vector3 cutPos = owner->m_gatling->GetAttachedPos();
			owner->m_gatling->Cut();
			EffectManager::GetInstance().Play(AsyncData::KingmechMissileExplodeEffect, cutPos, 0.0f, kCutEffectScale);
			owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kKneelStartAnim), false, owner->GetAnimSpeed(kKneelStartAnim));
			Shake(14.0f, 16.0f);
			FovPunch(5.0f, 16.0f);
		}
		if (m_frame >= kCutTime)ChangePhase(Phase::Throw);
		break;

	case Phase::Throw:
		UpdateKneel();
		if (!m_isEventDone && playerState && playerState->GetAnimFrame() >= kThrowFrame)
		{
			m_isEventDone = true;
			owner->m_gatling->ThrowAndStick();
		}
		if (owner->m_gatling->GetPhase() == KingmechGatling::Phase::Stuck)ChangePhase(Phase::Silence);
		break;

	case Phase::Silence:
		UpdateKneel();
		if (m_frame >= kSilenceTime)ChangePhase(Phase::Slash);
		break;

	case Phase::Slash:
		UpdateKneel();
		if (!m_isEventDone && m_frame >= kSlashHitFrame)
		{
			m_isEventDone = true;
			StartSlashHit();
		}
		screen.slashRate = m_isEventDone ? std::clamp(1.0f - (m_frame - kSlashHitFrame) / kSlashLineFrame, 0.0f, 1.0f) : 0.0f;
		if (m_frame >= kSlashTime)ChangePhase(Phase::Sheath);
		break;

	case Phase::Sheath:
		if (!m_isEventDone && m_frame >= kSheathFrame)
		{
			m_isEventDone = true;
			if (playerState)playerState->PlayAnim("Idle", true, 1.0f, -1.0f, 12.0f);
		}
		if (m_frame >= kSheathTime)ChangePhase(Phase::Split);
		break;

	case Phase::Split:
	{
		const float t = std::clamp(m_frame / kSplitTime, 0.0f, 1.0f);
		const float eased = Easing::Apply(EasingMode::EaseIn, t, 2.0f);
		owner->SetFinisherCutMove(kSlideLength * eased, kTiltMaxAngle * eased);
		if (m_frame >= kSplitTime)ChangePhase(Phase::Explode);
		break;
	}

	case Phase::Explode:
		screen.whiteRate = std::clamp(1.0f - m_frame / kExplodeFadeTime, 0.0f, 1.0f);
		if (m_frame >= kExplodeTime)Finish();
		break;

	case Phase::Done:
		//まだ先のフェーズがあるなら元に戻す//リザルトへ行くならこのまま暗転する
		if (!m_isCleaned && !battle->GetIsBattleEnd())
		{
			Cleanup();
			if (auto player = owner->m_player.lock())
			{
				if (auto camera = player->GetCameraManager().lock())
				{
					camera->ChangeStateFromScene(CameraManager::CameraStateName::PlayerCaemra);
				}
			}
		}
		break;
	}
}

void KingmechFinisher::Exit()
{
	Cleanup();
}

void KingmechFinisher::ChangePhase(Phase next)
{
	auto owner = GetBoss();
	if (!owner)return;
	auto playerState = GetPlayerState();
	if (!playerState)return;
	auto battle = System::GetInstance().GetBattleMgr();

	m_phase = next;
	m_frame = 0.0f;
	m_isEventDone = false;
	m_isSecondEventDone = false;

	const Vector3 bossChest = Point(0.0f, 0.0f, 480.0f);

	switch (m_phase)
	{
	case Phase::Supply:
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kRoarAnim), true, owner->GetAnimSpeed(kRoarAnim));
		SetShot(Point(1500.0f, 420.0f, 130.0f), Point(1350.0f, 520.0f, 90.0f),
			Point(0.0f, 0.0f, 500.0f), Point(0.0f, 0.0f, 620.0f), kFovWide, kFovWide - 6.0f, kSupplyTime);
		break;

	case Phase::Cut:
	{
		const Vector3 armPos = owner->m_gatling->GetAttachedPos();
		m_airPos = armPos + m_front * kAirFrontOffset;
		const ComboNode& node = playerState->GetComboNode(ComboIndex::GuardBreakAttack1);
		playerState->PlayAnim(node.animName, false, node.animTimeScale, node.endFrame, 6.0f, RootMotionCancel::Up);
		playerState->MoveTo(m_airPos, kLeapFrame, EasingMode::EaseOut, 2.0f);
		SetShot(Point(760.0f, -640.0f, 180.0f), Point(720.0f, -560.0f, 300.0f),
			armPos, armPos + Vector3(0.0f, 60.0f, 0.0f), kFovNormal, kFovNormal - 4.0f, kCutTime);
		break;
	}

	case Phase::Throw:
	{
		const ComboNode& node = playerState->GetComboNode(ComboIndex::AirAttack5);
		playerState->PlayAnim(node.animName, false, 0.3f, node.endFrame, node.blendFrame, RootMotionCancel::Down);
		const Vector3 camStart = m_airPos + m_front * 900.0f + m_side * 420.0f + Vector3(0.0f, -120.0f, 0.0f);
		SetShot(camStart, camStart + m_front * -80.0f, bossChest, bossChest, kFovNormal, kFovNormal - 6.0f, 110.0f);
		break;
	}

	case Phase::Silence:
	{
		System::GetInstance().SetTimeScale(kSilenceTimeScale);
		Shake(18.0f, 14.0f);
		FovPunch(-6.0f, 20.0f);
		const Vector3 stuck = owner->m_gatling->GetPos();
		SetShot(stuck + m_front * 560.0f + m_side * 230.0f + Vector3(0.0f, -40.0f, 0.0f),
			stuck + m_front * 430.0f + m_side * 170.0f + Vector3(0.0f, -20.0f, 0.0f),
			stuck, stuck, kFovClose, kFovClose - 4.0f, kSilenceTime);
		break;
	}

	case Phase::Slash:
		System::GetInstance().SetTimeScale(1.0f);
		battle->SetUltStart(kUltFrame, owner->m_gatling->GetPos());
		m_cutPoint = owner->m_gatling->GetPos();
		playerState->PlayAnim(playerState->GetAnimNameOf("DashAttack"), false, 1.2f, -1.0f, 3.0f);
		playerState->MoveTo(Point(-kPlayerEndBack, 0.0f, 0.0f), kSlashMoveFrame, EasingMode::EaseIn, 1.5f);
		SetShot(Point(1500.0f, 200.0f, 260.0f), Point(1460.0f, 200.0f, 250.0f),
			Point(0.0f, 0.0f, 400.0f), Point(0.0f, 0.0f, 400.0f), kFovWide, kFovWide, kSlashTime);
		break;

	case Phase::Sheath:
		playerState->Warp(Point(-kPlayerEndBack, 0.0f, 0.0f), m_front * -1.0f);
		playerState->PlayAnim(playerState->GetAnimNameOf("AttackPose"), true, 1.0f, -1.0f, 4.0f);
		SetShot(Point(-1230.0f, 280.0f, 60.0f), Point(-1130.0f, 250.0f, 60.0f),
			Point(-200.0f, 0.0f, 300.0f), Point(-200.0f, 0.0f, 320.0f), kFovWide, kFovWide - 4.0f, kSheathTime + kSplitTime + kExplodeTime);
		break;

	case Phase::Split:
		PlayWhiteBlood(kBloodScale);
		Shake(5.0f, kSplitTime);
		break;

	case Phase::Explode:
		owner->HideForFinisher();
		battle->SetUltEnd();
		battle->GetFinisherScreen().whiteRate = 1.0f;
		PlayExplosions();
		Shake(30.0f, 40.0f);
		FovPunch(8.0f, 30.0f);
		break;

	default:
		break;
	}
}

void KingmechFinisher::SetupStage()
{
	auto owner = GetBoss();
	if (!owner)return;
	auto playerState = GetPlayerState();
	if (!playerState)return;

	//最後の一撃の血殺・スローを消す
	System::GetInstance().GetBattleMgr()->SetUltEnd();
	System::GetInstance().SetTimeScaleForFrames(1.0f, 0);

	owner->m_missiles.clear();
	owner->m_gatling->ResetForFinisher();

	owner->m_rb.m_pos = m_center;
	owner->m_targetVec = m_front;
	owner->SetRotY(atan2f(m_front.x, m_front.z) - DX_PI_F);

	playerState->Warp(Point(kPlayerStartFront, 0.0f, 0.0f), m_front * -1.0f);
	playerState->PlayAnim(playerState->GetAnimNameOf("AttackPose"), true, 1.0f, -1.0f, 0.0f);

	if (auto player = owner->m_player.lock())
	{
		if (auto camera = player->GetCameraManager().lock())
		{
			camera->ChangeStateFromScene(CameraManager::CameraStateName::BossFinisherCamera);
		}
	}
}

void KingmechFinisher::StartSlashHit()
{
	auto owner = GetBoss();
	if (!owner)return;
	const Vector3 up(0.0f, 1.0f, 0.0f);
	const Vector3 normal = up * cosf(kCutTiltAngle) + m_side * sinf(kCutTiltAngle);
	const Vector3 slideDir = m_side * cosf(kCutTiltAngle) - up * sinf(kCutTiltAngle);
	owner->StartFinisherCut(normal, m_cutPoint, m_front, slideDir);
	PlayWhiteBlood(kBloodScale * 0.7f);
	Shake(24.0f, 18.0f);
	FovPunch(7.0f, 24.0f);
}

void KingmechFinisher::PlayWhiteBlood(float scale)
{
	const float rotY = atan2f(m_side.x, m_side.z);
	auto& effect = EffectManager::GetInstance();
	for (const float side : { -kBloodSideOffset, 0.0f, kBloodSideOffset })
	{
		const Vector3 pos = m_cutPoint + m_side * side;
		const int handle = effect.Play(AsyncData::BloodUltEffect, pos, rotY, scale);
		effect.SetOwnSpeed(handle, kBloodOwnSpeed);
	}
}

void KingmechFinisher::PlayExplosions()
{
	auto& effect = EffectManager::GetInstance();
	effect.Play(AsyncData::KingmechMissileExplodeEffect, m_cutPoint, 0.0f, kExplodeScale);
	effect.Play(AsyncData::GhostSkill3ImpactEffect, m_cutPoint, 0.0f, kExplodeScale);
	effect.Play(AsyncData::KingmechMissileExplodeEffect, Point(0.0f, kExplodeSubOffset, 120.0f), 0.0f, kExplodeSubScale);
	effect.Play(AsyncData::KingmechMissileExplodeEffect, Point(0.0f, -kExplodeSubOffset, 120.0f), 0.0f, kExplodeSubScale);
}

void KingmechFinisher::Finish()
{
	auto owner = GetBoss();
	if (!owner)return;
	m_phase = Phase::Done;
	System::GetInstance().SetTimeScaleForFrames(1.0f, 0);
	if (auto playerState = GetPlayerState())playerState->Finish();

	//ここで全滅扱いになってリザルトへ
	owner->m_isLifeZero = true;
	owner->m_isDead = true;
}

void KingmechFinisher::Cleanup()
{
	if (m_isCleaned)return;
	m_isCleaned = true;

	//シーンごと消えるときは戻す先が無い
	if (!System::GetInstance().HasBattleMgr())return;

	System::GetInstance().SetTimeScaleForFrames(1.0f, 0);
	auto battle = System::GetInstance().GetBattleMgr();
	battle->SetUltEnd();
	battle->SetIsEventPlaying(false);
	battle->GetFinisherScreen() = {};
}

void KingmechFinisher::SetShot(const Vector3& startPos, const Vector3& endPos, const Vector3& startTarget, const Vector3& endTarget,
	float startFov, float endFov, float frame)
{
	if (auto camera = GetCamera())
	{
		camera->SetShot({ startPos, endPos, startTarget, endTarget, startFov, endFov, frame });
	}
}

void KingmechFinisher::Shake(float power, float time)
{
	auto owner = GetBoss();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;
	if (auto camera = player->GetCameraManager().lock())camera->StartCameraShake(power, time);
}

void KingmechFinisher::FovPunch(float angleDeg, float time)
{
	auto owner = GetBoss();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;
	if (auto camera = player->GetCameraManager().lock())camera->StartFovPunch(angleDeg, time, 2.0f, 2.0f);
}

void KingmechFinisher::UpdateKneel()
{
	auto owner = GetBoss();
	if (!owner)return;
	if (owner->IsFinisherAnimStopped())return;
	if (m_phase == Phase::Cut && !m_isEventDone)return;//まだ吠えている
	if (!owner->m_anim.GetAnimEndFlag())return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kKneelLoopAnim), true, owner->GetAnimSpeed(kKneelLoopAnim));
}

Vector3 KingmechFinisher::Point(float front, float side, float height)const
{
	return m_center + m_front * front + m_side * side + Vector3(0.0f, height, 0.0f);
}

std::shared_ptr<BossEnemyKingmech> KingmechFinisher::GetBoss()const
{
	return std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
}

std::shared_ptr<PlayerStateBossFinisher> KingmechFinisher::GetPlayerState()const
{
	auto owner = GetBoss();
	if (!owner)return nullptr;
	auto player = owner->m_player.lock();
	if (!player)return nullptr;
	return player->GetBossFinisherState();
}

std::shared_ptr<BossFinisherCamera> KingmechFinisher::GetCamera()const
{
	auto owner = GetBoss();
	if (!owner)return nullptr;
	auto player = owner->m_player.lock();
	if (!player)return nullptr;
	auto camera = player->GetCameraManager().lock();
	if (!camera)return nullptr;
	return std::dynamic_pointer_cast<BossFinisherCamera>(camera->GetActiveCamera());
}
