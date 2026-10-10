#include "System.h"
#include "DxLib.h"
#include "EffekseerForDXLib.h"
#include "BattleManager.h"
#include <cassert>

namespace
{
	//プレイヤーのモデル
	//const std::string kPlayerModelPath = "data/2026_winter_Player_noY.mv1";
	const std::string kPlayerModelPath = "data/Player02.mv1";
	const std::string kPlayerWeaponModelPath = "data/Player_Weapon/katana_blend.mv1";//プレイヤーの刀
	//必殺技中の刀身の長い刀
	const std::string kPlayerUltWeaponModelPath = "data/Player_Weapon/red_katana_ult.mv1";

	const std::string kEnemyModelpath = "data/Enemy/Enemy.mv1";

	//部位破壊で落ちるパーツのモデル//原点はそれぞれのボーンの付け根
	const std::string kEnemyPartLeftArmModelPath = "data/Enemy/Part/Swordman/LeftArm.mv1";
	const std::string kEnemyPartHeadModelPath = "data/Enemy/Part/Swordman/Head.mv1";

	//ステージのモデル
	const std::string kTitleStageModelPath = "data/Stage/TestStage/TestStage.mv1";
	const std::string kDemoStageModelPath = "data/Stage/DemoStage/DemoStage.mv1";

	//スキルエフェクト確認用
	const std::string kDebugGhostDashEffectPath = "data/Effect/Skill/GhostDash.efk";
	const std::string kDebugGhostDash3DEffectPath = "data/Effect/Skill/GhostDash3D.efk";
	const std::string kGhostSkill3FallEffectPath = "data/Effect/Skill/GhostSkill3_Fall.efk";
	const std::string kGhostSkill3ImpactEffectPath = "data/Effect/Skill/GhostSkill3_Impact.efk";
	const std::string kBloodWingEffectPath = "data/Effect/Skill/BloodWing.efk";//

	//敵が斬られたときの血しぶき
	const std::string kBloodSplashAEffectPath = "data/Effect/Blood/BloodSplash.efk";
	const std::string kBloodSplashBEffectPath = "data/Effect/Blood/BloodSplash_B.efk";
	const std::string kBloodSplashCEffectPath = "data/Effect/Blood/BloodSplash_C.efk";
	const std::string kBloodSplashWhiteEffectPath = "data/Effect/Blood/BloodSplash_B_White.efk";//必殺技中
}

void System::LoadAll()
{
	//まだ無いもの(攻撃モデル・羽・武器・エフェクト)は-1にしておく
	for (int i = static_cast<int>(AsyncData::PlayerModel); i <= static_cast<int>(AsyncData::Goal); ++i)
	{
		m_asyncHandles[static_cast<AsyncData>(i)] = -1;
	}

	SetUseASyncLoadFlag(TRUE);//ここから下の読み込みは非同期になる

	m_asyncHandles[AsyncData::PlayerModel] = MV1LoadModel(kPlayerModelPath.c_str());
	m_asyncHandles[AsyncData::PlayerWeaponModel] = MV1LoadModel(kPlayerWeaponModelPath.c_str());
	m_asyncHandles[AsyncData::PlayerUltWeaponModel] = MV1LoadModel(kPlayerUltWeaponModelPath.c_str());
	m_asyncHandles[AsyncData::EnemyModel] = MV1LoadModel(kEnemyModelpath.c_str());
	m_asyncHandles[AsyncData::EnemyPartLeftArmModel] = MV1LoadModel(kEnemyPartLeftArmModelPath.c_str());
	m_asyncHandles[AsyncData::EnemyPartHeadModel] = MV1LoadModel(kEnemyPartHeadModelPath.c_str());
	m_asyncHandles[AsyncData::TitleStageModel] = MV1LoadModel(kTitleStageModelPath.c_str());
	m_asyncHandles[AsyncData::DemoStageModel] = MV1LoadModel(kDemoStageModelPath.c_str());

	SetUseASyncLoadFlag(FALSE);//ほかの場所の読み込みは同期に戻す

	//エフェクトは同期で読み込む
	m_asyncHandles[AsyncData::DebugGhostDashEffect] = LoadEffekseerEffect(kDebugGhostDashEffectPath.c_str());
	m_asyncHandles[AsyncData::DebugGhostDash3DEffect] = LoadEffekseerEffect(kDebugGhostDash3DEffectPath.c_str());
	m_asyncHandles[AsyncData::GhostSkill3FallEffect] = LoadEffekseerEffect(kGhostSkill3FallEffectPath.c_str());
	m_asyncHandles[AsyncData::GhostSkill3ImpactEffect] = LoadEffekseerEffect(kGhostSkill3ImpactEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodWingEffect] = LoadEffekseerEffect(kBloodWingEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodSplashEffectA] = LoadEffekseerEffect(kBloodSplashAEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodSplashEffectB] = LoadEffekseerEffect(kBloodSplashBEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodSplashEffectC] = LoadEffekseerEffect(kBloodSplashCEffectPath.c_str());
	m_asyncHandles[AsyncData::BloodSplashEffectWhite] = LoadEffekseerEffect(kBloodSplashWhiteEffectPath.c_str());
}

void System::Terminate()
{

	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerWeaponModel]);
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
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::EnemyHitEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::EnemyHitEffectUlt]);
	//ボス
	MV1DeleteModel(m_asyncHandles[AsyncData::BossModel]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BossAttackHadouEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BossAttackFireEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BossSummonEffect]);
	//mascot
	MV1DeleteModel(m_asyncHandles[AsyncData::MascotModel]);

	//stage
	MV1DeleteModel(m_asyncHandles[AsyncData::TitleStageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::DemoStageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::StageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::StageModelCollider]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::AreaWallEffect]);
	//デバッグ
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DebugGhostDashEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::DebugGhostDash3DEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::GhostSkill3FallEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::GhostSkill3ImpactEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodWingEffect]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodSplashEffectA]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodSplashEffectB]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodSplashEffectC]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::BloodSplashEffectWhite]);

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
			timeScale = 1.0f;//時間のスケールを元に戻す
			m_frameCount = -1;//フレームカウントを0にする
		}
	}
	
	auto battleMgr = m_battleMgr.lock();
	if (battleMgr)
	{
		battleMgr->Update();
	}

}

std::shared_ptr<BattleManager> System::GetBattleMgr()
{
	auto battleMgr =   m_battleMgr.lock(); 

	if (!battleMgr)assert(false);
	else return battleMgr;
}
