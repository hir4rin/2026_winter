#include "TunnelEnding.h"
#include "DxLib.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Camera/CameraManager.h"
#include "Camera/CameraState/EndingCamera.h"
#include "Character/Player/Player.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr int kWaitFrame = 90;//倒してから出口が見え始めるまで
	constexpr float kExitMinDistance = 2500.0f;//出口を少なくともこれより先に置く(トンネルの継ぎ目に置くので、実際はもっと先になる)
	constexpr float kApproachFrame = 300.0f;//出口が遠いときは列車を速くして、このフレーム数で着くようにする

	//出口が近づいたときのフォグ(前が青白く明るくなる)
	constexpr int kExitFogR = 45;
	constexpr int kExitFogG = 70;
	constexpr int kExitFogB = 105;
	constexpr float kExitFogStart = 600.0f;
	constexpr float kExitFogEnd = 4000.0f;
	//出口の穴の光(トンネルの中は背景を暗いままにする。明るくすると頭の上が空に見えてしまう)
	constexpr int kExitLightR = 215;
	constexpr int kExitLightG = 235;
	constexpr int kExitLightB = 250;
	constexpr float kExitLightBaseRate = 0.55f;//見え始めの明るさ

	//抜けた直後の霧
	constexpr int kWhiteOutFrame = 100;
	constexpr int kWhiteOutFogFrame = 30;//霧に包まれるまで
	constexpr int kWhiteOutCameraFrame = 30;//カメラが寄り始めるフレーム
	constexpr int kWhiteR = 175;
	constexpr int kWhiteG = 205;
	constexpr int kWhiteB = 235;
	constexpr float kWhiteFogStart = 0.0f;
	constexpr float kWhiteFogEnd = 700.0f;

	//霧が晴れた後の空(ステージ1の夜の空。色は空の絵の地平線の色)
	constexpr int kClearFrame = 240;
	constexpr int kSkyFogR = 48;
	constexpr int kSkyFogG = 21;
	constexpr int kSkyFogB = 54;
	constexpr float kSkyFogStart = 1500.0f;
	constexpr float kSkyFogEnd = 4950.0f;//カメラのファー5000より手前で消しきる
	constexpr int kSkyR = 48;
	constexpr int kSkyG = 21;
	constexpr int kSkyB = 54;

	constexpr int kGazeFrame = 200;//見上げたままの長さ

	//カメラ(EndingCamera::Shot)
	constexpr float kCloseBack = 220.0f;
	constexpr float kCloseHeight = 100.0f;
	constexpr float kCloseTargetAhead = 600.0f;
	constexpr float kCloseTargetHeight = 300.0f;
	constexpr float kCloseFov = 55.0f;
	//注視点を遠くにすると、注視点→カメラの線がステージに当たってカメラが前へ飛ぶので近めにする
	constexpr float kFarBack = 400.0f;
	constexpr float kFarHeight = 110.0f;
	constexpr float kFarTargetAhead = 500.0f;
	constexpr float kFarTargetHeight = 560.0f;
	constexpr float kFarFov = 60.0f;
	constexpr float kCloseDriftFrame = 400.0f;//見上げている間に少し寄る
	constexpr float kCloseDriftBack = 190.0f;
	constexpr float kPullBackFrame = 48.0f;//後ろへ引くフレーム数

	//クレジット
	constexpr int kCreditNum = 3;
	constexpr int kCreditStartFrame = 60;//カメラが引いてから最初のクレジットまで
	constexpr int kCreditLength = 216;//1枚が出ている長さ(EndingUIと同じにする)
	constexpr int kCreditGap = 42;//次の1枚まで
	constexpr int kLogoDelay = 30;//最後のクレジットからロゴまで
	constexpr int kLogoLength = 366;//ロゴが出ている長さ(EndingUIと同じにする)
}

TunnelEnding::TunnelEnding(std::weak_ptr<Stage> stage, std::weak_ptr<Player> player, std::weak_ptr<CameraManager> cameraManager) :
	m_stage(stage),
	m_player(player),
	m_cameraManager(cameraManager)
{
	if (auto stagePtr = m_stage.lock()) m_fog = stagePtr->GetTrainFog();
	m_fromFog = m_fog;
	if (auto battle = System::GetInstance().GetBattleMgr()) battle->GetEndingScreen().isPlaying = true;
}

TunnelEnding::~TunnelEnding()
{
	SetBackgroundColor(0, 0, 0);
}

void TunnelEnding::Update()
{
	auto battle = System::GetInstance().GetBattleMgr();
	auto stage = m_stage.lock();
	if (!battle || !stage)return;

	m_frame++;
	switch (m_phase)
	{
	case Phase::Wait:
		//血殺・演出が終わるまでは数えない
		if (battle->GetIsUltimating() || battle->GetIsEventPlaying()) m_frame = 0;
		if (m_frame >= kWaitFrame)
		{
			m_exitStartDistance = stage->StartTunnelExit(GetCameraPosition().z, kExitMinDistance);
			stage->SetTunnelSpeedScale((std::max)(1.0f, m_exitStartDistance / (stage->GetTunnelSpeed() * kApproachFrame)));
			ChangePhase(Phase::Approach);
		}
		break;
	case Phase::Approach:
		if (stage->GetTunnelExitDistance(GetCameraPosition().z) < 0.0f)
		{
			stage->SetTunnelSpeedScale(1.0f);
			ChangePhase(Phase::WhiteOut);
		}
		break;
	case Phase::WhiteOut:
		if (m_frame == kWhiteOutCameraFrame)
		{
			//プレイヤーを止めて、低い所から見上げる
			battle->SetIsEventPlaying(true);
			if (auto player = m_player.lock()) player->ForceIdleState();
			if (auto camera = m_cameraManager.lock()) camera->ChangeStateFromScene(CameraManager::CameraStateName::EndingCamera);
			if (auto camera = GetCamera())
			{
				camera->SetForward(Vector3(0.0f, 0.0f, stage->GetTrainForwardZ()));
				camera->MoveTo({ kCloseBack, kCloseHeight, kCloseTargetAhead, kCloseTargetHeight, kCloseFov }, 0.0f);
				camera->MoveTo({ kCloseDriftBack, kCloseHeight, kCloseTargetAhead, kCloseTargetHeight, kCloseFov }, kCloseDriftFrame);
			}
		}
		if (m_frame >= kWhiteOutFrame) ChangePhase(Phase::Clear);
		break;
	case Phase::Clear:
		if (m_frame >= kClearFrame) ChangePhase(Phase::Gaze);
		break;
	case Phase::Gaze:
		if (m_frame >= kGazeFrame)
		{
			//後ろへ引いて、また歩けるようにする
			if (auto camera = GetCamera()) camera->MoveTo({ kFarBack, kFarHeight, kFarTargetAhead, kFarTargetHeight, kFarFov }, kPullBackFrame);
			battle->SetIsEventPlaying(false);
			ChangePhase(Phase::Credit);
		}
		break;
	case Phase::Credit:
		if (m_frame >= kCreditStartFrame + kCreditNum * (kCreditLength + kCreditGap) + kLogoDelay) ChangePhase(Phase::Logo);
		break;
	case Phase::Logo:
		if (m_frame >= kLogoLength)
		{
			battle->SetIsBattleEnd(true);//リザルトへ
			ChangePhase(Phase::Done);
		}
		break;
	case Phase::Done:
		break;
	}

	UpdateFog();
	UpdateScreen();
}

void TunnelEnding::ChangePhase(Phase phase)
{
	m_phase = phase;
	m_frame = 0;
	m_fromFog = m_fog;
}

void TunnelEnding::UpdateFog()
{
	auto stage = m_stage.lock();
	if (!stage)return;

	const Stage::FogSetting exitFog = { kExitFogR, kExitFogG, kExitFogB, kExitFogStart, kExitFogEnd };
	const Stage::FogSetting whiteFog = { kWhiteR, kWhiteG, kWhiteB, kWhiteFogStart, kWhiteFogEnd };
	const Stage::FogSetting skyFog = { kSkyFogR, kSkyFogG, kSkyFogB, kSkyFogStart, kSkyFogEnd };

	switch (m_phase)
	{
	case Phase::Wait:
		return;
	case Phase::Approach:
	{
		//出口に近づくほど前が明るくなる
		const float rate = 1.0f - std::clamp(stage->GetTunnelExitDistance(GetCameraPosition().z) / m_exitStartDistance, 0.0f, 1.0f);
		m_fog = LerpFog(stage->GetTrainFog(), exitFog, rate * rate);
		const float light = kExitLightBaseRate + (1.0f - kExitLightBaseRate) * rate;
		stage->SetTunnelExitLight(static_cast<int>(kExitLightR * light), static_cast<int>(kExitLightG * light), static_cast<int>(kExitLightB * light));
		break;
	}
	case Phase::WhiteOut:
	{
		const float t = std::clamp(static_cast<float>(m_frame) / kWhiteOutFogFrame, 0.0f, 1.0f);
		m_fog = LerpFog(m_fromFog, whiteFog, t);
		m_bgR = kWhiteR;
		m_bgG = kWhiteG;
		m_bgB = kWhiteB;
		break;
	}
	case Phase::Clear:
	{
		float t = std::clamp(static_cast<float>(m_frame) / kClearFrame, 0.0f, 1.0f);
		t = t * t * (3.0f - 2.0f * t);
		m_fog = LerpFog(whiteFog, skyFog, t);
		m_bgR = static_cast<int>(std::lerp(static_cast<float>(kWhiteR), static_cast<float>(kSkyR), t));
		m_bgG = static_cast<int>(std::lerp(static_cast<float>(kWhiteG), static_cast<float>(kSkyG), t));
		m_bgB = static_cast<int>(std::lerp(static_cast<float>(kWhiteB), static_cast<float>(kSkyB), t));
		stage->SetOutsideRate(t);//霧が晴れるのに合わせて夜の空と遠くの街を出す
		break;
	}
	default:
		stage->SetOutsideRate(1.0f);
		break;
	}
	stage->SetFogOverride(m_fog);
	SetBackgroundColor(m_bgR, m_bgG, m_bgB);
}

void TunnelEnding::UpdateScreen()
{
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle)return;
	auto& screen = battle->GetEndingScreen();
	screen.creditIndex = -1;
	screen.logoFrame = -1;
	if (m_phase == Phase::Credit && m_frame >= kCreditStartFrame)
	{
		const int frame = m_frame - kCreditStartFrame;
		const int index = frame / (kCreditLength + kCreditGap);
		const int inCard = frame % (kCreditLength + kCreditGap);
		if (index < kCreditNum && inCard < kCreditLength)
		{
			screen.creditIndex = index;
			screen.creditFrame = inCard;
		}
	}
	if (m_phase == Phase::Logo)
	{
		screen.logoFrame = m_frame;
	}
}

std::shared_ptr<EndingCamera> TunnelEnding::GetCamera() const
{
	auto camera = m_cameraManager.lock();
	if (!camera)return nullptr;
	return std::dynamic_pointer_cast<EndingCamera>(camera->GetActiveCamera());
}

Stage::FogSetting TunnelEnding::LerpFog(const Stage::FogSetting& a, const Stage::FogSetting& b, float t)
{
	return {
		static_cast<int>(std::lerp(static_cast<float>(a.r), static_cast<float>(b.r), t)),
		static_cast<int>(std::lerp(static_cast<float>(a.g), static_cast<float>(b.g), t)),
		static_cast<int>(std::lerp(static_cast<float>(a.b), static_cast<float>(b.b), t)),
		std::lerp(a.start, b.start, t),
		std::lerp(a.end, b.end, t),
	};
}
