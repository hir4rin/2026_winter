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
	const std::string kStanceAnim = "AttackPose";
	const std::string kLeapAnim = "BossLeapSlash";//暗殺の踏み切りから空中で振りかぶり、急降下で振り下ろす(tools/PlayerAnim)

	//フェーズの長さ(実フレーム)
	constexpr float kStaggerTime = 30.0f;
	constexpr float kWhiteInTime = 12.0f;
	constexpr float kWhiteOutTime = 12.0f;
	constexpr float kStanceTime = 70.0f;//構え//飛び上がるまでの間
	constexpr float kBladeGrowEnd = 45.0f;//構えの中で刀身が伸びきる
	constexpr float kKessatsuTime = 50.0f;//止め絵
	constexpr float kSplitTime = 60.0f;
	constexpr float kRestTime = 70.0f;//割れた後、しゃがんだまま
	constexpr float kPullBackTime = 80.0f;//割れてから引く長さ

	constexpr float kStaggerTimeScale = 0.35f;
	constexpr float kApexTimeScale = 0.8f;//頂点で少し遅くする
	constexpr float kStrikeTimeScale = 0.5f;//着地の斬りを少し遅くする
	constexpr float kFreezeTimeScale = 0.02f;//止め絵の間
	constexpr int kUltFrame = 100000;//SetUltEndで終わらせる

	//飛び越え斬りのアニメ(Boss_LeapSlash、30fps で全74F)
	//0〜5Fしゃがんで踏み切り、12Fまでに振りかぶる、12〜22F頂点で止める、22〜26F振り下ろし、26〜30F刃を下に急降下、35Fで着地、36〜50Fしゃがみ、そのあと立つ
	constexpr float kAnimSpeed = 0.6f;
	constexpr float kDiveAnimSpeed = 1.2f;//急降下は速く
	constexpr float kCrouchAnimSpeed = 0.5f;//着地の後のしゃがみ
	constexpr float kCrouchHoldFrame = 46.0f;//しゃがんだまま止める(立ち上がらない)
	constexpr float kTakeoffAnimFrame = 5.0f;//踏み切り
	constexpr float kApexAnimFrame = 12.0f;//振りかぶりきる
	constexpr float kDiveAnimFrame = 22.0f;//振り下ろし始め//ここで急降下する
	constexpr float kStrikeAnimFrame = 30.0f;//着地の少し前//ここで真横のカメラへ
	constexpr float kLandAnimFrame = 35.0f;//着地//ここで止める

	//配置(闘技場の中央からの距離)
	constexpr float kStartFront = 330.0f;//構える位置(ボスのすぐ前)
	constexpr float kApexFront = 230.0f;//頂点(ボスの頭の前の上)
	constexpr float kApexSide = 60.0f;
	constexpr float kApexHeight = 650.0f;
	constexpr float kLandFront = 80.0f;//着地(ボスの真横)
	constexpr float kLandSide = 380.0f;

	//割れ方
	constexpr float kSplitTiltAngle = 15.0f * DX_PI_F / 180.0f;//外へ傾く角度//大きいとプレイヤー側の半分がプレイヤーに重なる
	constexpr float kSplitSlide = 60.0f;//外へずれる距離

	//血
	constexpr float kBloodScale = 45.0f;
	constexpr float kBloodOwnSpeed = 0.65f;
	constexpr float kBloodHeights[] = { 160.0f, 320.0f, 480.0f };//切り口に沿って出す高さ
	constexpr float kAbsorbEffectScale = 20.0f;//構えで刀に血を吸い込む

	//カメラ(手で詰める値はここ)//揺らさない・画角はほぼ変えない
	constexpr float kBackCamFront = 380.0f;//カメラ①: プレイヤーの後ろの距離
	constexpr float kBackCamSide = 110.0f;
	constexpr float kBackCamHeight = 170.0f;
	constexpr float kBackFov = 50.0f;
	constexpr float kFollowRate = 0.22f;//プレイヤーを追う速さ
	constexpr float kDiveFollowRate = 0.4f;
	constexpr float kFollowHeight = 110.0f;//プレイヤーの足元からの注視点の高さ
	//カメラ②: 着地したプレイヤーの真横(ボスと反対側)から//着地の斬り・止め絵はプレイヤーに寄り、割れるときに引く
	constexpr float kCloseCamDistance = 230.0f;//寄り
	constexpr float kCloseCamOut = 40.0f;//ボスから離れる向きへずらす
	constexpr float kCloseCamHeight = 70.0f;
	constexpr float kCloseTargetHeight = 120.0f;
	constexpr float kCloseCenterRate = 0.85f;//注視点をボス(0)〜プレイヤー(1)のどこにするか
	constexpr float kWideCamDistance = 430.0f;//引き
	constexpr float kWideCamOut = 120.0f;
	constexpr float kWideCamHeight = 70.0f;
	constexpr float kWideTargetHeight = 210.0f;
	constexpr float kWideCenterRate = 0.7f;
	constexpr float kSideFov = 50.0f;
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
			ChangePhase(Phase::Stance);
		}
		break;

	case Phase::Stance:
		UpdateKneel();
		screen.whiteRate = std::clamp(1.0f - m_frame / kWhiteOutTime, 0.0f, 1.0f);
		playerState->SetUltBladeRate(std::clamp(m_frame / kBladeGrowEnd, 0.0f, 1.0f));
		if (m_frame >= kStanceTime)ChangePhase(Phase::Rise);
		break;

	case Phase::Rise:
		UpdateKneel();
		//踏み切ったら真上へ
		if (!m_isEventDone && playerState->GetAnimFrame() >= kTakeoffAnimFrame)
		{
			m_isEventDone = true;
			const float riseTime = (kApexAnimFrame - kTakeoffAnimFrame) / kAnimSpeed;
			playerState->MoveTo(Point(kApexFront, kApexSide, kApexHeight), riseTime, EasingMode::EaseOut, 2.5f);
		}
		FollowPlayer(kFollowRate);
		if (playerState->GetAnimFrame() >= kApexAnimFrame)ChangePhase(Phase::Apex);
		break;

	case Phase::Apex:
		UpdateKneel();
		FollowPlayer(kFollowRate);
		if (playerState->GetAnimFrame() >= kDiveAnimFrame)ChangePhase(Phase::Dive);
		break;

	case Phase::Dive:
		UpdateKneel();
		FollowPlayer(kDiveFollowRate);
		if (playerState->GetAnimFrame() >= kStrikeAnimFrame)ChangePhase(Phase::Strike);
		break;

	case Phase::Strike:
		if (playerState->GetAnimFrame() >= kLandAnimFrame)ChangePhase(Phase::Kessatsu);
		break;

	case Phase::Kessatsu:
		if (m_frame >= kKessatsuTime)ChangePhase(Phase::Split);
		break;

	case Phase::Split:
	{
		const float t = std::clamp(m_frame / kSplitTime, 0.0f, 1.0f);
		const float eased = Easing::Apply(EasingMode::EaseIn, t, 2.5f);
		owner->SetFinisherCutMove(kSplitSlide * eased, kSplitTiltAngle * eased);
		if (m_frame >= kSplitTime)ChangePhase(Phase::Rest);
		break;
	}

	case Phase::Rest:
		if (m_frame >= kRestTime)Finish();
		break;

	case Phase::Done:
		//まだ先のフェーズがあるなら元に戻す//リザルトへ行くならこのまま暗転する
		if (!m_isCleaned && !battle->GetIsBattleEnd())
		{
			owner->HideForFinisher();
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

	const Vector3 backCam = Point(kStartFront + kBackCamFront, kBackCamSide, kBackCamHeight);

	switch (m_phase)
	{
	case Phase::Stance:
		//カメラ①: プレイヤーの後ろから膝をついたボスを見る//急降下までこの位置のまま
		playerState->PlayAnim(playerState->GetAnimNameOf(kStanceAnim), true, 1.0f, -1.0f, 0.0f);
		playerState->SetUltSword(true);
		playerState->SetUltBladeRate(0.0f);
		EffectManager::GetInstance().Play(AsyncData::UltBloodAbsorbEffect, Point(kStartFront, 0.0f, 100.0f), 0.0f, kAbsorbEffectScale);
		SetShot(backCam, backCam, Point(0.0f, 0.0f, 280.0f), Point(0.0f, 0.0f, 280.0f), kBackFov, kBackFov, kStanceTime);
		break;

	case Phase::Rise:
		//同じ位置のまま、注視点だけプレイヤーを追って見上げる
		playerState->PlayAnim(playerState->GetAnimNameOf(kLeapAnim), false, kAnimSpeed, -1.0f, 3.0f);
		break;

	case Phase::Apex:
		System::GetInstance().SetTimeScale(kApexTimeScale);
		break;

	case Phase::Dive:
	{
		//ボスの真横へ急降下//着地まで(着地の斬りのスローも入れて)ちょうど降りきる
		System::GetInstance().SetTimeScale(1.0f);
		const float animFrame = playerState->GetAnimFrame();
		playerState->PlayAnim(playerState->GetAnimNameOf(kLeapAnim), false, kDiveAnimSpeed, -1.0f, 0.0f, RootMotionCancel::None, animFrame);
		playerState->SetFront(m_landDir);
		const float diveTime = (kStrikeAnimFrame - animFrame) / kDiveAnimSpeed +
			(kLandAnimFrame - kStrikeAnimFrame) / (kDiveAnimSpeed * kStrikeTimeScale);
		playerState->MoveTo(m_landPos, diveTime, EasingMode::EaseIn, 1.4f);
		break;
	}

	case Phase::Strike:
		//カメラ②: 低い真横//ボスが左、プレイヤーが右の手前//止め絵の後もこの構図
		System::GetInstance().SetTimeScale(kStrikeTimeScale);
		SetShot(m_sideCamPos, m_sideCamPos, m_sideCamTarget, m_sideCamTarget, kSideFov, kSideFov, 1.0f);
		break;

	case Phase::Kessatsu:
		battle->SetUltStart(kUltFrame, m_center + Vector3(0.0f, 380.0f, 0.0f));
		System::GetInstance().SetTimeScale(kFreezeTimeScale);
		break;

	case Phase::Split:
	{
		//止め絵が終わったらすぐ割れる//寄りから、割れるのが見える引きまでゆっくり下がる
		System::GetInstance().SetTimeScale(1.0f);
		StartCut();
		playerState->PlayAnim(playerState->GetAnimNameOf(kLeapAnim), false, kCrouchAnimSpeed, kCrouchHoldFrame, 0.0f, RootMotionCancel::None, playerState->GetAnimFrame());
		PlayCutBlood(kBloodScale);
		SetShot(m_sideCamPos, m_wideCamPos, m_sideCamTarget, m_wideCamTarget, kSideFov, kSideFov + 2.0f, kPullBackTime);
		break;
	}

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
	//腕に付いていなければ消す//付いていればボスと一緒に割れる
	if (owner->m_gatling->GetPhase() != KingmechGatling::Phase::Attached)owner->m_gatling->ResetForFinisher();

	owner->m_rb.m_pos = m_center;
	owner->m_targetVec = m_front;
	owner->SetRotY(atan2f(m_front.x, m_front.z) - DX_PI_F);

	playerState->Warp(Point(kStartFront, 0.0f, 0.0f), m_front * -1.0f);

	//着地はボスの真横//着地したプレイヤーはボスに背を向ける
	m_landPos = Point(kLandFront, kLandSide, 0.0f);
	Vector3 toLand = m_landPos - m_center;
	toLand.y = 0.0f;
	m_landDir = toLand.Normalize();

	//カメラ②: 着地したプレイヤーの真横から//この向きならボスが左、プレイヤーが右に映る
	const Vector3 up(0.0f, 1.0f, 0.0f);
	const Vector3 camSide = up.Cross(m_landDir).Normalize();
	m_sideCamPos = m_landPos + camSide * kCloseCamDistance + m_landDir * kCloseCamOut + up * kCloseCamHeight;
	m_sideCamTarget = m_center + (m_landPos - m_center) * kCloseCenterRate + up * kCloseTargetHeight;
	m_wideCamPos = m_landPos + camSide * kWideCamDistance + m_landDir * kWideCamOut + up * kWideCamHeight;
	m_wideCamTarget = m_center + (m_landPos - m_center) * kWideCenterRate + up * kWideTargetHeight;

	if (auto player = owner->m_player.lock())
	{
		if (auto camera = player->GetCameraManager().lock())
		{
			camera->ChangeStateFromScene(CameraManager::CameraStateName::BossFinisherCamera);
		}
	}
}

void KingmechFinisher::StartCut()
{
	auto owner = GetBoss();
	if (!owner)return;
	//真横のカメラの見る向きと上を含む面で割る//画面の左右に倒れる
	Vector3 view = m_sideCamTarget - m_sideCamPos;
	view.y = 0.0f;
	const Vector3 viewDir = view.Normalize();
	const Vector3 normal = Vector3(0.0f, 1.0f, 0.0f).Cross(viewDir).Normalize();
	owner->StartFinisherCut(normal, m_center, viewDir);
}

void KingmechFinisher::PlayCutBlood(float scale)
{
	//切り口からプレイヤーの側へ噴く
	const float rotY = atan2f(m_landDir.x, m_landDir.z);
	auto& effect = EffectManager::GetInstance();
	for (const float height : kBloodHeights)
	{
		const Vector3 pos = m_center + Vector3(0.0f, height, 0.0f);
		const int handle = effect.Play(AsyncData::BloodUltEffect, pos, rotY, scale);
		effect.SetOwnSpeed(handle, kBloodOwnSpeed);
	}
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

void KingmechFinisher::UpdateKneel()
{
	auto owner = GetBoss();
	if (!owner)return;
	if (owner->IsFinisherAnimStopped())return;
	if (!owner->m_anim.GetAnimEndFlag())return;
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kKneelLoopAnim), true, owner->GetAnimSpeed(kKneelLoopAnim));
}

void KingmechFinisher::FollowPlayer(float rate)
{
	auto playerState = GetPlayerState();
	if (!playerState)return;
	if (auto camera = GetCamera())camera->SetFollowTarget(playerState->GetPos() + Vector3(0.0f, kFollowHeight, 0.0f), rate);
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
