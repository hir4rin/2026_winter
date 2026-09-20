#include "System.h"
#include "DxLib.h"
#include "EffekseerForDXLib.h"


void System::SetTerminate()
{

	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerAttackModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerWeaponModel]);
	MV1DeleteModel(m_asyncHandles[AsyncData::PlayerWingModel]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerEffectSkill]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerEffectSkill2]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerEffectSkill3]);
	DeleteEffekseerEffect(m_asyncHandles[AsyncData::PlayerHitEffect]);

	//enemy
	MV1DeleteModel(m_asyncHandles[AsyncData::EnemyModel]);
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

	
		m_ultCount -= 1.0f * timeScale;

		if (m_ultCount <= 0)
		{
			m_ultCount = -1;
			m_isUltimating = false;
		}
	

}
