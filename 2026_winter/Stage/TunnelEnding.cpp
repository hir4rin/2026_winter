#include "TunnelEnding.h"
#include "DxLib.h"
#include "EffekseerForDXLib.h"
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
	constexpr int kWalkFrame = 150;//ロゴが消えてから囲まれるまで歩ける長さ

	//跳ぶ向き(床の端)を探す
	constexpr float kEdgeProbeStep = 20.0f;//床の端を探す刻み
	constexpr float kEdgeProbeMax = 1200.0f;//これより先は探さない
	constexpr float kEdgeProbeUp = 80.0f;//足元のこれだけ上から下へ線を引く
	constexpr float kEdgeProbeDown = 150.0f;
	constexpr float kEdgeChestHeight = 120.0f;//跳ぶ向きに物が無いか調べる高さ

	//囲まれる(フレームは演出が始まってから。実フレーム)
	const char* const kWarningEffectPath = "data/Effect/Kingmech/MissileWarning.efk";//予告の円(45Fで着弾の時間)
	const char* const kFallEffectPath = "data/Effect/Kingmech/MissileFall.efk";//落ちてくる(15F)
	const char* const kExplodeEffectPath = "data/Effect/Kingmech/MissileExplode.efk";
	const char* const kLeapBurstEffectPath = "data/Effect/Train/TrainLeapBurst.efk";//煙を突き抜ける(+Z が飛び出す向き)
	const char* const kFallTrailEffectPath = "data/Effect/Train/TrainFallTrail.efk";//落ちる間の煙の尾・火の粉・風の筋(プレイヤーについていく)
	constexpr float kTrainEffectScale = 20.0f;//1/20で作ったエフェクト
	constexpr int kMissileFallFrame = 30;//予告から落ちてくるまで
	constexpr int kMissileHitFrame = 45;//予告から着弾まで
	constexpr int kRingNum = 7;//まわりのミサイルの数
	constexpr int kRingStartFrame = 15;//1発目の予告
	constexpr int kRingInterval = 9;//予告を出す間隔
	constexpr float kRingSide = 230.0f;//まわりの楕円の横の半径(車両の幅に収める)
	constexpr float kRingAlong = 650.0f;//まわりの楕円の前後の半径
	constexpr float kRingScale = 20.0f;//1/20で作ったエフェクト
	constexpr float kRingShakePower = 6.0f;
	constexpr float kRingShakeTime = 12.0f;
	constexpr int kCenterStartFrame = 70;//最後にプレイヤーの足元へ
	constexpr float kCenterScale = 32.0f;//大きい爆発で包む
	constexpr float kCenterSideBlast = 170.0f;//前後にも爆発を足して厚くする
	constexpr float kCenterShakePower = 22.0f;
	constexpr float kCenterShakeTime = 40.0f;
	constexpr int kCenterHitFrame = kCenterStartFrame + kMissileHitFrame;//最後の大きい爆発
	constexpr int kHideFrame = kCenterHitFrame + 4;//炎に包まれたらプレイヤーを隠す
	constexpr int kWideCutFrame = kCenterHitFrame + 22;//引きに切り替えて、煙の上がる列車を映す(やられたか…の間)
	constexpr int kBeatRadioFrame = kCenterHitFrame + 40;//無線「応答して！」
	constexpr int kLeapFrame = kCenterHitFrame + 95;//一呼吸おいて、煙の中から飛び出す
	//引きのカメラ(列車の外の、屋根より少し高い所から)
	constexpr float kWideOut = 1500.0f;//跳ぶ向きへ
	constexpr float kWideAhead = 650.0f;//列車の前へ
	constexpr float kWideUp = 40.0f;//上へ//見上げ気味にして空を背にする
	//やられたか…の間も燃えているように、爆発を重ねる
	constexpr int kBurnFrames[] = { kWideCutFrame, kWideCutFrame + 35 };
	constexpr float kBurnScale = 24.0f;
	constexpr float kBurnHeight = 80.0f;
	constexpr float kLeapBurstScale = 28.0f;//飛び出す瞬間の爆風
	//カメラ(クレーンで引き上がって見下ろす)
	constexpr float kCraneOut = 450.0f;//跳ぶ向きへ
	constexpr float kCraneAhead = 300.0f;//列車の前へ
	constexpr float kCraneUp = 1200.0f;//上へ//真上寄りから見下ろして、車両の上の物に隠れないようにする
	constexpr float kCraneFrame = 70.0f;//引き上がる長さ

	//飛び出した後(フレームは飛び出してから。実フレーム)
	constexpr float kLeapTimeScale = 0.3f;//飛び出す瞬間のスロー
	constexpr int kLeapSlowFrame = 45;
	constexpr int kFollowStartFrame = 75;//落ちていくのについていき始める(それまでは引きで見る)
	constexpr float kFollowBlendFrame = 45.0f;//引きの位置から寄りへ動く長さ
	constexpr float kCloseOut = 420.0f;//寄りのカメラ(プレイヤーから跳ぶ向きへ)//車両の外を通るように離す
	constexpr float kCloseAhead = 160.0f;
	constexpr float kCloseUp = 60.0f;//少し上から
	constexpr int kJumpBlackStart = 130;//暗くなり始める
	constexpr int kJumpBlackFrame = 24;//真っ黒になるまで
	constexpr int kJumpEndFrame = kJumpBlackStart + kJumpBlackFrame + 2;//リザルトへ

	constexpr int kRadioAmbush = 0;//無線の字幕の番号(EndingUIの文と合わせる)
	constexpr int kRadioBeat = 1;
}

TunnelEnding::TunnelEnding(std::weak_ptr<Stage> stage, std::weak_ptr<Player> player, std::weak_ptr<CameraManager> cameraManager) :
	m_stage(stage),
	m_player(player),
	m_cameraManager(cameraManager)
{
	if (auto stagePtr = m_stage.lock()) m_fog = stagePtr->GetTrainFog();
	m_fromFog = m_fog;
	if (auto battle = System::GetInstance().GetBattleMgr()) battle->GetEndingScreen().isPlaying = true;
	m_warningEffect = LoadEffekseerEffect(kWarningEffectPath);
	m_fallEffect = LoadEffekseerEffect(kFallEffectPath);
	m_explodeEffect = LoadEffekseerEffect(kExplodeEffectPath);
	m_leapBurstEffect = LoadEffekseerEffect(kLeapBurstEffectPath);
	m_fallTrailEffect = LoadEffekseerEffect(kFallTrailEffectPath);
}

TunnelEnding::~TunnelEnding()
{
	SetBackgroundColor(0, 0, 0);
	for (const auto& missile : m_missiles)
	{
		if (missile.warning != -1) StopEffekseer3DEffect(missile.warning);
	}
	DeleteEffekseerEffect(m_warningEffect);
	DeleteEffekseerEffect(m_fallEffect);
	DeleteEffekseerEffect(m_explodeEffect);
	if (m_fallTrailPlaying != -1) StopEffekseer3DEffect(m_fallTrailPlaying);
	DeleteEffekseerEffect(m_leapBurstEffect);
	DeleteEffekseerEffect(m_fallTrailEffect);
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
		if (m_frame >= kLogoLength) ChangePhase(Phase::Walk);
		break;
	case Phase::Walk:
		//少し歩かせてから囲まれる
		if (m_frame >= kWalkFrame && !battle->GetIsEventPlaying()) StartAmbush();
		break;
	case Phase::Ambush:
		UpdateAmbush();
		break;
	case Phase::Jump:
		//落ちる間の煙の尾・風の筋をプレイヤーにつける
		if (m_fallTrailPlaying != -1)
		{
			if (auto player = m_player.lock())
			{
				const Vector3 foot = player->GetRigidBody().GetPos();
				SetPosPlayingEffekseer3DEffect(m_fallTrailPlaying, foot.x, foot.y, foot.z);
			}
		}
		if (m_frame == kFollowStartFrame)
		{
			//クレーンの位置から、落ちていくプレイヤーの横へ寄っていく
			const Vector3 forward = Vector3(0.0f, 0.0f, stage->GetTrainForwardZ());
			if (auto camera = GetCamera()) camera->FollowAt(m_jumpOutDir * kCloseOut + forward * kCloseAhead + Vector3(0.0f, kCloseUp, 0.0f), kFollowBlendFrame);
		}
		if (m_frame >= kJumpEndFrame)
		{
			battle->SetIsBattleEnd(true);//リザルトへ(「次へ」でチュートリアル)
			if (m_fallTrailPlaying != -1) StopEffekseer3DEffect(m_fallTrailPlaying);
			m_fallTrailPlaying = -1;
			ChangePhase(Phase::Done);
		}
		break;
	case Phase::Done:
		break;
	}

	if (m_radioIndex >= 0) m_radioFrame++;
	UpdateMissiles();
	UpdateFog();
	UpdateScreen();
}

void TunnelEnding::StartAmbush()
{
	auto battle = System::GetInstance().GetBattleMgr();
	auto player = m_player.lock();
	auto stage = m_stage.lock();
	if (!battle || !player || !stage)return;

	//止めて、まわりにミサイルの予告を並べる
	battle->SetIsEventPlaying(true);
	player->ForceIdleState();
	FindJumpEdge();
	m_radioIndex = kRadioAmbush;
	m_radioFrame = 0;

	const Vector3 foot = player->GetRigidBody().GetPos();
	const Vector3 forward = Vector3(0.0f, 0.0f, stage->GetTrainForwardZ());
	const Vector3 out = m_jumpOutDir;
	const float side = (std::min)(kRingSide, m_edgeDist);
	m_missiles.clear();
	m_missileClock = 0;
	m_ambushCenter = foot;
	for (int i = 0; i < kRingNum; ++i)
	{
		//楕円の上をぐるっと回る順に出す
		const float angle = DX_TWO_PI_F * i / kRingNum + DX_PI_F * 0.5f;
		const Vector3 pos = foot + out * (cosf(angle) * side) + forward * (sinf(angle) * kRingAlong);
		m_missiles.push_back({ pos, kRingStartFrame + kRingInterval * i, kRingScale, false });
	}
	m_missiles.push_back({ foot, kCenterStartFrame, kCenterScale, true });

	//クレーンで引き上がって、囲まれているのを見下ろす(ロゴの間のカメラから続ける)
	if (!GetCamera())
	{
		if (auto camera = m_cameraManager.lock()) camera->ChangeStateFromScene(CameraManager::CameraStateName::EndingCamera);
	}
	if (auto camera = GetCamera()) camera->HoldAt(foot + out * kCraneOut + forward * kCraneAhead + Vector3(0.0f, kCraneUp, 0.0f), kCraneFrame);
	ChangePhase(Phase::Ambush);
}

void TunnelEnding::UpdateAmbush()
{
	auto player = m_player.lock();
	auto stage = m_stage.lock();
	if (!player || !stage)return;

	//炎に包まれたら隠す(やられたか…に見せる)
	if (m_frame == kHideFrame) player->SetHidden(true);
	//引きに切り替えて、煙の上がる列車を外から映す
	if (m_frame == kWideCutFrame)
	{
		const Vector3 forward = Vector3(0.0f, 0.0f, stage->GetTrainForwardZ());
		const Vector3 foot = player->GetRigidBody().GetPos();
		if (auto camera = GetCamera()) camera->HoldAt(foot + m_jumpOutDir * kWideOut + forward * kWideAhead + Vector3(0.0f, kWideUp, 0.0f), 0.0f);
	}
	for (int frame : kBurnFrames)
	{
		if (m_frame == frame) PlayEffect(m_explodeEffect, m_ambushCenter + Vector3(0.0f, kBurnHeight, 0.0f), kBurnScale);
	}
	if (m_frame == kBeatRadioFrame)
	{
		m_radioIndex = kRadioBeat;
		m_radioFrame = 0;
	}
	//一呼吸おいて、爆風の中から飛び出す
	if (m_frame >= kLeapFrame)
	{
		PlayEffect(m_explodeEffect, m_ambushCenter, kLeapBurstScale);
		if (auto camera = m_cameraManager.lock()) camera->StartCameraShake(kCenterShakePower, kCenterShakeTime);
		player->SetHidden(false);
		StartJump();
	}
}

void TunnelEnding::UpdateMissiles()
{
	if (m_missiles.empty())return;
	m_missileClock++;
	auto play = [](int effect, const Vector3& pos, float scale) { return PlayEffect(effect, pos, scale); };
	for (auto& missile : m_missiles)
	{
		const int local = m_missileClock - missile.startFrame;
		if (local == 0) missile.warning = play(m_warningEffect, missile.pos, missile.scale);
		if (local == kMissileFallFrame) play(m_fallEffect, missile.pos, missile.scale);
		if (local == kMissileHitFrame)
		{
			if (missile.warning != -1) StopEffekseer3DEffect(missile.warning);
			missile.warning = -1;
			play(m_explodeEffect, missile.pos, missile.scale);
			if (missile.isCenter)
			{
				//前後にも重ねて、プレイヤーを炎と煙で包む
				float forwardZ = 1.0f;
				if (auto stage = m_stage.lock()) forwardZ = stage->GetTrainForwardZ();
				const Vector3 along = Vector3(0.0f, 0.0f, forwardZ * kCenterSideBlast);
				play(m_explodeEffect, missile.pos + along, kRingScale);
				play(m_explodeEffect, missile.pos - along, kRingScale);
			}
			if (auto camera = m_cameraManager.lock())
			{
				if (missile.isCenter) camera->StartCameraShake(kCenterShakePower, kCenterShakeTime);
				else camera->StartCameraShake(kRingShakePower, kRingShakeTime);
			}
		}
	}
}

int TunnelEnding::PlayEffect(int effect, const Vector3& pos, float scale)
{
	const int handle = PlayEffekseer3DEffect(effect);
	SetPosPlayingEffekseer3DEffect(handle, pos.x, pos.y, pos.z);
	SetScalePlayingEffekseer3DEffect(handle, scale, scale, scale);
	return handle;
}

void TunnelEnding::StartJump()
{
	auto player = m_player.lock();
	auto stage = m_stage.lock();
	if (!player || !stage)return;

	//爆風の中から列車の外へ飛び出す(一瞬スロー)
	const Vector3 forward = Vector3(0.0f, 0.0f, stage->GetTrainForwardZ());
	player->StartEndingJump(m_jumpOutDir, forward * -1.0f);
	System::GetInstance().SetTimeScaleForFrames(kLeapTimeScale, kLeapSlowFrame);
	//煙を突き抜ける(飛び出す向きへ煙と火の粉を押し出す)
	const int burst = PlayEffect(m_leapBurstEffect, m_ambushCenter, kTrainEffectScale);
	SetRotationPlayingEffekseer3DEffect(burst, 0.0f, atan2f(m_jumpOutDir.x, m_jumpOutDir.z), 0.0f);
	//落ちる間の煙の尾・火の粉・風の筋(位置は Jump の間ずっとプレイヤーに合わせる)
	m_fallTrailPlaying = PlayEffect(m_fallTrailEffect, player->GetRigidBody().GetPos(), kTrainEffectScale);
	ChangePhase(Phase::Jump);
}

void TunnelEnding::FindJumpEdge()
{
	auto player = m_player.lock();
	auto stage = m_stage.lock();
	if (!player || !stage)return;

	const int model = stage->GetStageModelHandle();
	const Vector3 foot = player->GetRigidBody().GetPos();
	float bestDist = kEdgeProbeMax;
	float bestSide = 1.0f;
	bool bestBlocked = true;
	for (float side : { 1.0f, -1.0f })
	{
		float edge = kEdgeProbeMax;
		for (float d = 0.0f; d <= kEdgeProbeMax; d += kEdgeProbeStep)
		{
			const VECTOR from = VGet(foot.x + side * d, foot.y + kEdgeProbeUp, foot.z);
			const VECTOR to = VGet(foot.x + side * d, foot.y - kEdgeProbeDown, foot.z);
			if (!MV1CollCheck_Line(model, -1, from, to).HitFlag)
			{
				edge = d;
				break;
			}
		}
		//胸の高さで端の外まで線を引いて、コンテナなどにぶつかる側は避ける
		const VECTOR chest = VGet(foot.x, foot.y + kEdgeChestHeight, foot.z);
		const VECTOR outside = VGet(foot.x + side * (edge + kEdgeProbeStep * 5.0f), foot.y + kEdgeChestHeight, foot.z);
		const bool isBlocked = MV1CollCheck_Line(model, -1, chest, outside).HitFlag != 0;
		const bool isBetter = (isBlocked == bestBlocked) ? edge < bestDist : !isBlocked;
		if (isBetter)
		{
			bestDist = edge;
			bestSide = side;
			bestBlocked = isBlocked;
		}
	}
	m_jumpOutDir = Vector3(bestSide, 0.0f, 0.0f);
	m_edgeDist = bestDist;
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
	screen.radioIndex = m_radioIndex;
	screen.radioFrame = m_radioFrame;
	screen.blackRate = (m_phase == Phase::Jump || m_phase == Phase::Done) ?
		std::clamp(static_cast<float>(m_frame - kJumpBlackStart) / kJumpBlackFrame, 0.0f, 1.0f) : 0.0f;
	if (m_phase == Phase::Done) screen.blackRate = 1.0f;
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
