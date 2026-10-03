#pragma once
#include <unordered_map>
//#include "Sound/SoundManager.h"
#include <memory>

class BattleManager;


//非同期ロードリストの種類//ここ治す
enum class AsyncData : int
{
	//プレイヤー
	PlayerModel,
	PlayerAttackModel,							
	PlayerWeaponModel,
	PlayerWingModel,
	PlayerEffectSkill,
	PlayerEffectSkill2 = 5,
	PlayerEffectSkill3,
	PlayerHitEffect,
	PlayerSwordHitEffect,
	PlayerSwordHitEffect2,
	PlayerDamageEffect,
	JumpAttackFootEffect,
	//敵
	EnemyModel,
	EnemyHitEffect,
	EnemyHitEffectUlt,
	EnemyPartLeftArmModel,//部位破壊で落ちる左腕
	EnemyPartHeadModel,//部位破壊で落ちる頭(ヘルメット)
	//ボス
	BossModel,
	BossAttackHadouEffect,
	BossAttackFireEffect,
	BossSummonEffect,
	//マスコット
	MascotModel,
	//ステージ
	TitleStageModel,
	StageModel,
	StageModelCollider,
	DemoStageModel,//デモステージ(UnityのDemoStageBuilderで生成)
	AreaWallEffect,
	WallBreakEffect,
	Goal,
};

class System
{
private:
	//コンストラクタとデストラクタをプライベートにして、シングルトンパターンを実装
	System() = default;
	virtual ~System() = default;
	//コピーコンストラクタと代入演算子を削除して、シングルトンのインスタンスが複製されないようにする
	System(const System&) = delete;
	System& operator=(const System&) = delete;
public:
	//シングルトンインスタンスを取得
	static System& GetInstance()
	{
		static System instance;
		return instance;
	}

	/// <summary>
	/// 読み込んだモデル・エフェクトを解放する(DxLib_Endより前に呼ぶ)
	/// </summary>
	void Terminate();

	void SetTimeScale(float scale) { timeScale = scale; }
	float GetTimeScale() const { return timeScale; }
	/// <summary>
	/// 時間の流れともとに戻るまでのフレーム数を設定する関数
	/// </summary>
	/// <param name="timescale">時間の流れ=通常時が1.0f</param>
	/// <param name="frames">通常時に戻るまでのframe数</param>
	void SetTimeScaleForFrames(float timescale, int frames)
	{
		timeScale = timescale;//時間のスケールを設定する
		m_frameCount = frames;//フレームカウントを設定する
	}

	/// <summary>
	/// モデル・エフェクトを非同期で読み込み、ハンドルを保存する
	/// </summary>
	/// <note>戻った時点では読み込みが終わっていないので、GetASyncLoadNum()が0になるまで待ってから使う</note>
	void LoadAll();
	int GetHandle(AsyncData key) { return m_asyncHandles[key]; }

	void Update();
	//バトルマネージャー
	void SetBattleMgr(std::weak_ptr<BattleManager> mgr) { m_battleMgr = mgr; }
	std::shared_ptr<BattleManager> GetBattleMgr();

	//SoundManager& GetSoundManager() { return m_soundManager; }

private:
	//時間の管理
	float timeScale = 1.0f;//時間のスケール//1.0fなら通常の時間の流れ//0.5fなら半分の速さ//2.0fなら2倍の速さ

	//いじった時間をもとに戻すためのフレーム
	int m_frameCount = -1;//フレームカウント//ゲームが開始してからのフレーム数//0から始まる

	std::weak_ptr<BattleManager> m_battleMgr;//バトルマネージャー

	std::unordered_map<AsyncData, int> m_asyncHandles; //非同期ロードのハンドルを保持するマップ

	//SoundManager m_soundManager;//サウンドマネージャー//GetSoundManager()経由でPlayBgm等を呼ぶ
};

