#include "System.h"
#include "DxLib.h"
#include "EffekseerForDXLib.h"
#include "BattleManager.h"
#include <cassert>

namespace
{
	//プレイヤーのモデル
	const std::string kPlayerModelPath = "data/2026_winter_Player_noY.mv1";

	const std::string kEnemyModelpath = "data/Enemy/Enemy.mv1";

	//部位破壊で落ちるパーツのモデル//原点はそれぞれのボーンの付け根
	const std::string kEnemyPartLeftArmModelPath = "data/Enemy/Part/Swordman/LeftArm.mv1";
	const std::string kEnemyPartHeadModelPath = "data/Enemy/Part/Swordman/Head.mv1";

	//ステージのモデル
	const std::string kTitleStageModelPath = "data/Stage/TestStage/TestStage.mv1";
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
	m_asyncHandles[AsyncData::EnemyModel] = MV1LoadModel(kEnemyModelpath.c_str());
	m_asyncHandles[AsyncData::EnemyPartLeftArmModel] = MV1LoadModel(kEnemyPartLeftArmModelPath.c_str());
	m_asyncHandles[AsyncData::EnemyPartHeadModel] = MV1LoadModel(kEnemyPartHeadModelPath.c_str());
	m_asyncHandles[AsyncData::TitleStageModel] = MV1LoadModel(kTitleStageModelPath.c_str());

	SetUseASyncLoadFlag(FALSE);//ほかの場所の読み込みは同期に戻す
}

void System::Terminate()
{

	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerWeaponModel]);
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
	MV1DeleteModel(m_asyncHandles[AsyncData::StageModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::StageModelCollider]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::AreaWallEffect]);

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
