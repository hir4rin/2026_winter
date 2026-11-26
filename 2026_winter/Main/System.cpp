#include "System.h"
#include "DxLib.h"
#include "EffekseerForDXLib.h"
#include "Battle/BattleManager.h"
#include "Render/CharacterLook.h"
#include "DataLoader/DataManager.h"
#include "Stage/StageInfo.h"
#include "FrameProfiler.h"
#include <cassert>

namespace
{
	//プレイヤーのモデル
	//const std::string kPlayerModelPath = "data/2026_winter_Player_noY.mv1";
	//const std::string kPlayerModelPath = "data/Player02.mv1";
	const std::string kPlayerLookCsvPath = "data/Player/Look/PlayerLook.csv";//Player.cpp の kLookCsvPath と同じにする
	const std::string kPlayerModelPath = "data/Player03.mv1";	const std::string kPlayerWeaponModelPath = "data/Player_Weapon/katana_blend.mv1";//プレイヤーの刀
	//必殺技中の刀身の長い刀
	const std::string kPlayerUltWeaponModelPath = "data/Player_Weapon/red_katana_ult.mv1";

	const std::string kEnemyModelpath = "data/Enemy/Enemy.mv1";
	const std::string kEnemyGunnerModelPath = "data/Enemy/EnemyGun.mv1";
	const std::string kEnemyGunnerWeaponModelPath = "data/Enemy/EnemyGun/AK74.mv1";//ガンナーの銃
	const std::string kEnemyGunnerPartLeftArmModelPath = "data/Enemy/Part/Gunner/LeftArm.mv1";//ソードマンの左腕から盾を外したもの
	const std::string kEnemyBazookaWeaponModelPath = "data/Enemy/EnemyBazooka/RPG7.mv1";//バズーカ兵のRPG7
	const std::string kBossModelPath = "data/Enemy/BossPirate/BossPirate.mv1";//ボス(海賊)
	const std::string kBossArmLModelPath = "data/Enemy/BossPirate/BossArmL.mv1";//ボスの背中の左腕//フレームの並びはボスと同じ
	const std::string kBossArmRModelPath = "data/Enemy/BossPirate/BossArmR.mv1";//ボスの背中の右腕
	const std::string kBossPirateSwordModelPath = "data/Enemy/BossPirate/PirateSword.mv1";//海賊のボスの剣//Unity の Pirate_Sword.FBX から作った
	const std::string kKingmechModelPath = "data/Enemy/ToiletMech/ToiletMech.mv1";//新しいボス(Kingmech)//tools/ToiletMech で作った
	const std::string kKingmechGatlingModelPath = "data/Enemy/ToiletMech/KingmechGatling.mv1";//Kingmechのガトリング//tools/KingmechGatling で作った

	//部位破壊で落ちるパーツのモデル//原点はそれぞれのボーンの付け根
	const std::string kEnemyPartLeftArmModelPath = "data/Enemy/Part/Swordman/LeftArm.mv1";
	const std::string kEnemyPartHeadModelPath = "data/Enemy/Part/Swordman/Head.mv1";

	//ステージのモデル
	const std::string kTitleStageModelPath = "data/Stage/TestStage/TestStage.mv1";
	const std::string kDemoStageModelPath = "data/Stage/DemoStage/DemoStage.mv1";
	const std::string kTrainStageModelPath = "data/Stage/TrainStage/TrainStage.mv1";
	const std::string kTrainStageColModelPath = "data/Stage/TrainStage/TrainStageCol.mv1";
	const std::string kTrainTunnelModelPath = "data/Stage/TrainTunnel/TrainTunnel.mv1";
	const std::string kTrainTrackModelPath = "data/Stage/TrainTunnel/TrainTrack.mv1";
	const std::string kCyberStageModelPath = "data/Stage/CyberStage/CyberStage.mv1";
	const std::string kCyberStageColModelPath = "data/Stage/CyberStage/CyberStageCol.mv1";
	const std::string kTutorialStageModelPath = "data/Stage/TutorialStage/TutorialStage.mv1";
	const std::string kTutorialStageColModelPath = "data/Stage/TutorialStage/TutorialStageCol.mv1";

	//スキルエフェクト確認用
	const std::string kDebugGhostDashEffectPath = "data/Effect/Skill/GhostDash.efk";
	const std::string kDebugGhostDash3DEffectPath = "data/Effect/Skill/GhostDash3D.efk";
	const std::string kDebugGleamLight1EffectPath = "data/Effect/Skill/GleamLight1.efk";
	const std::string kDebugGleamLight2EffectPath = "data/Effect/Skill/GleamLight2.efk";
	const std::string kDebugGleamLight3EffectPath = "data/Effect/Skill/GleamLight3.efk";
	const std::string kGhostSkill3FallEffectPath = "data/Effect/Skill/GhostSkill3_Fall.efk";
	const std::string kGhostSkill3ImpactEffectPath = "data/Effect/Skill/GhostSkill3_Impact.efk";
	const std::string kGhostLaunchEffectPath = "data/Effect/Skill/GhostLaunch.efk";
	const std::string kBloodWingEffectPath = "data/Effect/Skill/BloodWing.efk";//
	const std::string kBloodWingActivateEffectPath = "data/Effect/Skill/BloodWingActivate.efk";

	//敵が斬られたときの血しぶき
	const std::string kBloodHitEffectPath = "data/Effect/Blood/BloodHit.efk";//通常ヒット
	const std::string kBloodHitCEffectPath = "data/Effect/Blood/BloodHit_C.efk";
	const std::string kBloodHitDEffectPath = "data/Effect/Blood/BloodHit_D.efk";
	const std::string kBloodAssassinEffectPath = "data/Effect/Blood/BloodAssassin.efk";
	const std::string kBloodKillEffectPath = "data/Effect/Blood/BloodKill.efk";
	const std::string kBloodPartBreakEffectPath = "data/Effect/Blood/BloodPartBreak.efk";
	const std::string kBloodUltEffectPath = "data/Effect/Blood/BloodUlt.efk";
	const std::string kUltBloodAbsorbEffectPath = "data/Effect/Blood/UltBloodAbsorb.efk";
	//ガードブレイク
	const std::string kGuardBreakFlashEffectPath = "data/Effect/GuardBreak/GuardBreakFlash.efk";
	const std::string kGuardBreakShardsEffectPath = "data/Effect/GuardBreak/GuardBreakShards.efk";
	//斬ったときの風
	const std::string kHitWindEffectPath = "data/Effect/Hit/HitWind.efk";
	const std::string kSkillHitEffectPath = "data/Effect/Hit/SkillHit.efk";
	//地上からのジャンプ
	const std::string kJumpKickEffectPath = "data/Effect/Move/JumpKick.efk";
	//ダッシュ
	const std::string kDashStartEffectPath = "data/Effect/Move/DashStart.efk";
	const std::string kDashSparkleEffectPath = "data/Effect/Move/DashSparkle.efk";
	//チェックポイント
	const std::string kCheckpointIdleEffectPath = "data/Effect/Checkpoint/CheckpointIdle.efk";
	const std::string kCheckpointActivateEffectPath = "data/Effect/Checkpoint/CheckpointActivate.efk";
	//回復
	const std::string kHealEffectPath = "data/Effect/Heal/HealSpiral.efk";
	//溜め攻撃
	const std::string kChargeEffectPath = "data/Effect/ChargeAttack/Charge.efk";
	const std::string kChargeSlash1EffectPath = "data/Effect/ChargeAttack/Slash1.efk";
	const std::string kChargeSlash2EffectPath = "data/Effect/ChargeAttack/Slash2.efk";
	const std::string kChargeChargingEffectPath = "data/Effect/ChargeHold/W1_Charging.efk";
	const std::string kChargeDoneEffectPath = "data/Effect/ChargeHold/W1_Done.efk";
	const std::string kChargeChargedEffectPath = "data/Effect/ChargeHold/W1_Charged.efk";
	//溜め中のエフェクトの案//AsyncData::ChargeHoldA2Effect からの並び
	const char* const kChargeHoldPaths[] = {
		"data/Effect/ChargeHold/A2_Hold.efk",
		"data/Effect/ChargeHold/A3_Hold.efk",
		"data/Effect/ChargeHold/A4_Hold.efk",
		"data/Effect/ChargeHold/A5_Hold.efk",
		"data/Effect/ChargeHold/C_Hold.efk",
		"data/Effect/ChargeHold/D_Hold.efk",
		"data/Effect/ChargeHold/D2_Hold.efk",
		"data/Effect/ChargeHold/D3_Hold.efk",
		"data/Effect/ChargeHold/D4_Hold.efk",
		"data/Effect/ChargeHold/D5_Hold.efk",
		"data/Effect/ChargeHold/E_Hold.efk",
		"data/Effect/ChargeHold/F_Hold.efk",
		"data/Effect/ChargeHold/H_Hold.efk",
		"data/Effect/ChargeHold/J_Hold.efk",
		"data/Effect/ChargeHold/K_Hold.efk",
		"data/Effect/ChargeHold/L_Hold.efk",
	};
	//F11 で切り替える案の名前//0 は今の溜め(Charge.efk)
	const char* const kChargeHoldNames[] = {
		"A 赤黒の瘴気(今)", "A2 吸う→噴く", "A3 刃が黒く染まる", "A4 煙の色が変わる", "A5 吸う＋染まる", "C 風を呑む", "D 紅の闘気(火の粉が集まる)", "D2 黒い炎→紅に着火", "D3 だんだん大きく", "D4 刀から燃え移る", "D5 鼓動", "E 赤い雷", "F 重圧・地割れ",
		"H 鬼の眼光", "J 残像が重なる", "K 動画の型(青→赤)", "L 白銀→紅",
	};

	//Kingmechのミサイルの雨
	const std::string kKingmechMissileWarningEffectPath = "data/Effect/Kingmech/MissileWarning.efk";
	const std::string kKingmechMissileFallEffectPath = "data/Effect/Kingmech/MissileFall.efk";
	const std::string kKingmechMissileExplodeEffectPath = "data/Effect/Kingmech/MissileExplode.efk";
	//Kingmechの攻撃
	const std::string kKingmechGatlingExplodeEffectPath = "data/Effect/Kingmech/GatlingExplode.efk";
	const std::string kKingmechSweepWindEffectPath = "data/Effect/Kingmech/SweepWind.efk";
	const std::string kKingmechGroundPunchEffectPath = "data/Effect/Kingmech/GroundPunch.efk";
	const std::string kKingmechDashTrailEffectPath = "data/Effect/Kingmech/DashTrail.efk";
	const std::string kKingmechDashImpactEffectPath = "data/Effect/Kingmech/DashImpact.efk";
	const std::string kEnemyBazookaMissileEffectPath = "data/Effect/Bazooka/BazookaMissile.efk";
}

void System::LoadCommon()
{
	//まだ無いもの(羽・エフェクト・ステージの分)は-1にしておく
	for (int i = static_cast<int>(AsyncData::PlayerModel); i <= static_cast<int>(AsyncData::Goal); ++i)
	{
		m_asyncHandles[static_cast<AsyncData>(i)] = -1;
	}

	SetUseASyncLoadFlag(TRUE);//ここから下の読み込みは非同期になる

	m_asyncHandles[AsyncData::PlayerModel] = MV1LoadModel(kPlayerModelPath.c_str());	m_asyncHandles[AsyncData::PlayerWeaponModel] = MV1LoadModel(kPlayerWeaponModelPath.c_str());
	m_asyncHandles[AsyncData::PlayerUltWeaponModel] = MV1LoadModel(kPlayerUltWeaponModelPath.c_str());
	//プレイヤーの法線マップ・マスク画像(重いので先に読む。Player の Load はこれを使い回す)
	CharacterLook::PreloadGraphs(kPlayerLookCsvPath);

	SetUseASyncLoadFlag(FALSE);//ほかの場所の読み込みは同期に戻す

	//エフェクトは同期で読み込む
	m_asyncHandles[AsyncData::DebugGhostDashEffect] = LoadEffekseerEffect(kDebugGhostDashEffectPath.c_str());
	m_asyncHandles[AsyncData::DebugGhostDash3DEffect] = LoadEffekseerEffect(kDebugGhostDash3DEffectPath.c_str());
	m_asyncHandles[AsyncData::DebugGleamLight1Effect] = LoadEffekseerEffect(kDebugGleamLight1EffectPath.c_str());
	m_asyncHandles[AsyncData::DebugGleamLight2Effect] = LoadEffekseerEffect(kDebugGleamLight2EffectPath.c_str());
	m_asyncHandles[AsyncData::DebugGleamLight3Effect] = LoadEffekseerEffect(kDebugGleamLight3EffectPath.c_str());
	m_asyncHandles[AsyncData::GhostSkill3FallEffect] = LoadEffekseerEffect(kGhostSkill3FallEffectPath.c_str());
	m_asyncHandles[AsyncData::GhostSkill3ImpactEffect] = LoadEffekseerEffect(kGhostSkill3ImpactEffectPath.c_str());
	m_asyncHandles[AsyncData::GhostLaunchEffect] = LoadEffekseerEffect(kGhostLaunchEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodWingEffect] = LoadEffekseerEffect(kBloodWingEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodWingActivateEffect] = LoadEffekseerEffect(kBloodWingActivateEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodHitEffect] = LoadEffekseerEffect(kBloodHitEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodHitEffectC] = LoadEffekseerEffect(kBloodHitCEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodHitEffectD] = LoadEffekseerEffect(kBloodHitDEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodAssassinEffect] = LoadEffekseerEffect(kBloodAssassinEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodKillEffect] = LoadEffekseerEffect(kBloodKillEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodPartBreakEffect] = LoadEffekseerEffect(kBloodPartBreakEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodUltEffect] = LoadEffekseerEffect(kBloodUltEffectPath.c_str());
	m_asyncHandles[AsyncData::UltBloodAbsorbEffect] = LoadEffekseerEffect(kUltBloodAbsorbEffectPath.c_str());
	m_asyncHandles[AsyncData::GuardBreakFlashEffect] = LoadEffekseerEffect(kGuardBreakFlashEffectPath.c_str());
	m_asyncHandles[AsyncData::GuardBreakShardsEffect] = LoadEffekseerEffect(kGuardBreakShardsEffectPath.c_str());
	m_asyncHandles[AsyncData::HitWindEffect] = LoadEffekseerEffect(kHitWindEffectPath.c_str());
	m_asyncHandles[AsyncData::SkillHitEffect] = LoadEffekseerEffect(kSkillHitEffectPath.c_str());
	m_asyncHandles[AsyncData::JumpKickEffect] = LoadEffekseerEffect(kJumpKickEffectPath.c_str());
	m_asyncHandles[AsyncData::DashStartEffect] = LoadEffekseerEffect(kDashStartEffectPath.c_str());
	m_asyncHandles[AsyncData::DashSparkleEffect] = LoadEffekseerEffect(kDashSparkleEffectPath.c_str());
	m_asyncHandles[AsyncData::CheckpointIdleEffect] = LoadEffekseerEffect(kCheckpointIdleEffectPath.c_str());
	m_asyncHandles[AsyncData::CheckpointActivateEffect] = LoadEffekseerEffect(kCheckpointActivateEffectPath.c_str());
	m_asyncHandles[AsyncData::HealEffect] = LoadEffekseerEffect(kHealEffectPath.c_str());
	m_asyncHandles[AsyncData::ChargeChargeEffect] = LoadEffekseerEffect(kChargeEffectPath.c_str());
	m_asyncHandles[AsyncData::ChargeSlash1Effect] = LoadEffekseerEffect(kChargeSlash1EffectPath.c_str());
	m_asyncHandles[AsyncData::ChargeSlash2Effect] = LoadEffekseerEffect(kChargeSlash2EffectPath.c_str());
	m_asyncHandles[AsyncData::ChargeChargingEffect] = LoadEffekseerEffect(kChargeChargingEffectPath.c_str());
	m_asyncHandles[AsyncData::ChargeDoneEffect] = LoadEffekseerEffect(kChargeDoneEffectPath.c_str());
	m_asyncHandles[AsyncData::ChargeChargedEffect] = LoadEffekseerEffect(kChargeChargedEffectPath.c_str());
	for (int i = 0; i < static_cast<int>(std::size(kChargeHoldPaths)); ++i)
	{
		m_asyncHandles[static_cast<AsyncData>(static_cast<int>(AsyncData::ChargeHoldA2Effect) + i)] = LoadEffekseerEffect(kChargeHoldPaths[i]);
	}
}

const char* System::GetChargeHoldName() const
{
	return kChargeHoldNames[m_chargeHoldType];
}

AsyncData System::GetChargeHoldEffectKey() const
{
	if (m_chargeHoldType == 0) return AsyncData::ChargeChargeEffect;
	return static_cast<AsyncData>(static_cast<int>(AsyncData::ChargeHoldA2Effect) + m_chargeHoldType - 1);
}

void System::LoadStage(StageType type)
{
	if (m_loadedStage == static_cast<int>(type)) return;
	ReleaseStage();
	m_loadedStage = static_cast<int>(type);

	//同期で読む(非同期は ProcessMessage を呼んだときにしか進まない処理があり、1フレームに1回だと何倍も遅い)
	switch (type)
	{
	case StageType::Test:
		LoadStageModel(AsyncData::TitleStageModel, kTitleStageModelPath);
		break;
	case StageType::Demo:
		LoadStageModel(AsyncData::DemoStageModel, kDemoStageModelPath);
		break;
	case StageType::Train:
		LoadStageModel(AsyncData::TrainStageModel, kTrainStageModelPath);
		LoadStageModel(AsyncData::TrainStageColModel, kTrainStageColModelPath);
		LoadStageModel(AsyncData::TrainTunnelModel, kTrainTunnelModelPath);
		LoadStageModel(AsyncData::TrainTrackModel, kTrainTrackModelPath);
		break;
	case StageType::Cyber:
		LoadStageModel(AsyncData::CyberStageModel, kCyberStageModelPath);
		LoadStageModel(AsyncData::CyberStageColModel, kCyberStageColModelPath);
		break;
	case StageType::Tutorial:
		LoadStageModel(AsyncData::TutorialStageModel, kTutorialStageModelPath);
		LoadStageModel(AsyncData::TutorialStageColModel, kTutorialStageColModelPath);
		break;
	default:
		break;
	}
	//ソードマンはデバッグの敵出し(F)と、配置の無いステージのテスト用の敵で使うのでいつも読む
	LoadEnemy(EnemyType::Swordman);
	DataManager::GetInstance().LoadEnemySpawnData(StageData::GetInfo(type).enemySpawnNumber);
	for (const auto& spawn : DataManager::GetInstance().GetEnemySpawnData())
	{
		LoadEnemy(spawn.type);
	}
}

void System::LoadStageModel(AsyncData key, const std::string& path)
{
	if (m_asyncHandles[key] != -1) return;
	m_asyncHandles[key] = MV1LoadModel(path.c_str());
	m_stageModels.push_back(key);
}

void System::LoadStageEffect(AsyncData key, const std::string& path)
{
	if (m_asyncHandles[key] != -1) return;
	m_asyncHandles[key] = LoadEffekseerEffect(path.c_str());
	m_stageEffects.push_back(key);
}

void System::LoadEnemy(EnemyType type)
{
	switch (type)
	{
	case EnemyType::Swordman:
		LoadStageModel(AsyncData::EnemyModel, kEnemyModelpath);
		LoadStageModel(AsyncData::EnemyPartLeftArmModel, kEnemyPartLeftArmModelPath);
		LoadStageModel(AsyncData::EnemyPartHeadModel, kEnemyPartHeadModelPath);
		break;
	case EnemyType::Gunner:
		LoadStageModel(AsyncData::EnemyGunnerModel, kEnemyGunnerModelPath);
		LoadStageModel(AsyncData::EnemyGunnerWeaponModel, kEnemyGunnerWeaponModelPath);
		LoadStageModel(AsyncData::EnemyGunnerPartLeftArmModel, kEnemyGunnerPartLeftArmModelPath);
		LoadStageModel(AsyncData::EnemyPartHeadModel, kEnemyPartHeadModelPath);
		break;
	case EnemyType::Bazooka:
		//体・左腕・頭はガンナーと同じ
		LoadStageModel(AsyncData::EnemyGunnerModel, kEnemyGunnerModelPath);
		LoadStageModel(AsyncData::EnemyBazookaWeaponModel, kEnemyBazookaWeaponModelPath);
		LoadStageModel(AsyncData::EnemyGunnerPartLeftArmModel, kEnemyGunnerPartLeftArmModelPath);
		LoadStageModel(AsyncData::EnemyPartHeadModel, kEnemyPartHeadModelPath);
		//ミサイルの予告と爆発はKingmechのものを使う
		LoadStageEffect(AsyncData::KingmechMissileWarningEffect, kKingmechMissileWarningEffectPath);
		LoadStageEffect(AsyncData::KingmechMissileExplodeEffect, kKingmechMissileExplodeEffectPath);
		LoadStageEffect(AsyncData::EnemyBazookaMissileEffect, kEnemyBazookaMissileEffectPath);
		break;
	case EnemyType::BossEnemyPirateR1:
		//第二ラウンドは背中の腕も使う
		LoadStageModel(AsyncData::BossArmLModel, kBossArmLModelPath);
		LoadStageModel(AsyncData::BossArmRModel, kBossArmRModelPath);
		[[fallthrough]];
	case EnemyType::BossEnemyPirateR0:
		LoadStageModel(AsyncData::BossModel, kBossModelPath);
		LoadStageModel(AsyncData::BossPirateSwordModel, kBossPirateSwordModelPath);
		LoadStageModel(AsyncData::EnemyPartLeftArmModel, kEnemyPartLeftArmModelPath);
		LoadStageModel(AsyncData::EnemyPartHeadModel, kEnemyPartHeadModelPath);
		break;
	case EnemyType::BossEnemyKingmech:
		LoadStageModel(AsyncData::KingmechModel, kKingmechModelPath);
		LoadStageModel(AsyncData::KingmechGatlingModel, kKingmechGatlingModelPath);
		LoadStageEffect(AsyncData::KingmechMissileWarningEffect, kKingmechMissileWarningEffectPath);
		LoadStageEffect(AsyncData::KingmechMissileFallEffect, kKingmechMissileFallEffectPath);
		LoadStageEffect(AsyncData::KingmechMissileExplodeEffect, kKingmechMissileExplodeEffectPath);
		LoadStageEffect(AsyncData::KingmechGatlingExplodeEffect, kKingmechGatlingExplodeEffectPath);
		LoadStageEffect(AsyncData::KingmechSweepWindEffect, kKingmechSweepWindEffectPath);
		LoadStageEffect(AsyncData::KingmechGroundPunchEffect, kKingmechGroundPunchEffectPath);
		LoadStageEffect(AsyncData::KingmechDashTrailEffect, kKingmechDashTrailEffectPath);
		LoadStageEffect(AsyncData::KingmechDashImpactEffect, kKingmechDashImpactEffectPath);
		break;
	default:
		break;
	}
}

void System::ReleaseStage()
{
	for (AsyncData key : m_stageModels)
	{
		MV1DeleteModel(m_asyncHandles[key]);
		m_asyncHandles[key] = -1;
	}
	for (AsyncData key : m_stageEffects)
	{
		DeleteEffekseerEffect(m_asyncHandles[key]);
		m_asyncHandles[key] = -1;
	}
	m_stageModels.clear();
	m_stageEffects.clear();
	m_loadedStage = -1;
}

void System::Terminate()
{

	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerModel]);	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerWeaponModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerUltWeaponModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerWingModel]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerEffectSkill]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerEffectSkill2]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerEffectSkill3]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerHitEffect]);

	//enemy
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyPartLeftArmModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyPartHeadModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyGunnerModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyGunnerWeaponModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyGunnerPartLeftArmModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyBazookaWeaponModel]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::EnemyHitEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::EnemyHitEffectUlt]);
	//ボス
	MV1DeleteModel(m_asyncHandles[AsyncData::BossModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::BossArmLModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::BossArmRModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::BossPirateSwordModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::KingmechModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::KingmechGatlingModel]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BossAttackHadouEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BossAttackFireEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BossSummonEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechMissileWarningEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechMissileFallEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechMissileExplodeEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechGatlingExplodeEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechSweepWindEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechGroundPunchEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechDashTrailEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::KingmechDashImpactEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::EnemyBazookaMissileEffect]);
	//mascot
	MV1DeleteModel(m_asyncHandles[AsyncData::MascotModel]);

	//stage
	MV1DeleteModel(m_asyncHandles[AsyncData::TitleStageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::DemoStageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::TrainStageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::TrainStageColModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::TrainTunnelModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::TrainTrackModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::CyberStageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::CyberStageColModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::TutorialStageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::TutorialStageColModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::StageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::StageModelCollider]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::AreaWallEffect]);
	//デバッグ
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DebugGhostDashEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DebugGhostDash3DEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DebugGleamLight1Effect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DebugGleamLight2Effect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DebugGleamLight3Effect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::GhostSkill3FallEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::GhostSkill3ImpactEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::GhostLaunchEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodWingEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodWingActivateEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodHitEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodHitEffectC]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodHitEffectD]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodAssassinEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodKillEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodPartBreakEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodUltEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::UltBloodAbsorbEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::GuardBreakFlashEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::GuardBreakShardsEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::HitWindEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::SkillHitEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::JumpKickEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DashStartEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DashSparkleEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::CheckpointIdleEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::CheckpointActivateEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::HealEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::ChargeChargeEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::ChargeSlash1Effect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::ChargeSlash2Effect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::ChargeChargingEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::ChargeDoneEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::ChargeChargedEffect]);
	for (int i = 0; i < static_cast<int>(std::size(kChargeHoldPaths)); ++i)
	{
		DeleteEffekseerEffect(m_asyncHandles[static_cast<AsyncData>(static_cast<int>(AsyncData::ChargeHoldA2Effect) + i)]);
	}

}

void System::SetTimeScale(float scale, std::source_location location)
{
	if (scale != timeScale) FrameProfiler::GetInstance().RecordTimeScale(timeScale, scale, location.file_name(), static_cast<int>(location.line()));
	timeScale = scale;
}

void System::SetTimeScaleForFrames(float timescale, int frames, std::source_location location)
{
	SetTimeScale(timescale, location);//時間のスケールを設定する
	m_frameCount = frames;//フレームカウントを設定する
}

void System::Update()
{
	//サウンドのフェードなどを更新する
	//m_soundManager.Update();

	//m_frameCountが-1(カウントダウンしていない)なら、timeScaleの自動リセットは行わない
	//これをしないと、SetTimeScaleで直接セットしたtimeScaleが次フレームで1.0fに戻されてしまう
	if (m_frameCount >= 0)
	{
		m_frameCount--;

		if (m_frameCount <= 0)
		{
			SetTimeScale(1.0f);//時間のスケールを元に戻す
			m_frameCount = -1;//フレームカウントを0にする
		}
	}
}

std::shared_ptr<BattleManager> System::GetBattleMgr()
{
	auto battleMgr =   m_battleMgr.lock(); 

	if (!battleMgr)assert(false);
	else return battleMgr;
}
