#pragma once
#include <unordered_map>
//#include "Sound/SoundManager.h"


//非同期ロードリストの種類
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

	void SetTerminate();

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

	//非同期ロードのハンドルを保存する
	void SetHandleData(std::unordered_map<AsyncData, int> data) { m_asyncHandles = data; }
	int GetHandle(AsyncData key) { return m_asyncHandles[key]; }

	void Update();

	void SetUltStart(int frames = -1) { m_ultCount = frames; m_isUltimating = true; };
	bool GetIsUltimating() { return m_isUltimating; };
	void SetUltEnd() { m_ultCount = -1; m_isUltimating = false; };

	void SetPhotoMode(bool ans) { m_isPhotoMode = ans; }
	bool GetPhotoMode() { return m_isPhotoMode; }

	//イベント演出中(カメラ演出+その戻りのBlend中)はプレイヤー/敵の入力・行動を止めるためのフラグ
	void SetIsEventPlaying(bool ans) { m_isEventPlaying = ans; }
	bool GetIsEventPlaying() { return m_isEventPlaying; }

	//ラストヒットのイベント中かどうか
	void SetIsLastHitEventPlaying(bool ans) { m_isLastHitEventPlaying = ans;}
	bool GetIsLastHitEventPlaying(){return m_isLastHitEventPlaying;}
	//falseから初めてtrueになった瞬間だけ、内部でtrueにする(多重呼び出し防止)
	void SetIsLastHitEventPlayingTrigger(bool ans);

	//SoundManager& GetSoundManager() { return m_soundManager; }



private:
	//時間の管理
	float timeScale = 1.0f;//時間のスケール//1.0fなら通常の時間の流れ//0.5fなら半分の速さ//2.0fなら2倍の速さ

	//いじった時間をもとに戻すためのフレーム
	int m_frameCount = -1;//フレームカウント//ゲームが開始してからのフレーム数//0から始まる

	bool m_isUltimating = false;//必殺技の演出中かどうか
	int m_ultCount = -1;//必殺技の演出時間カウント

	bool m_isPhotoMode = false;

	bool m_isEventPlaying = false;//イベント演出中かどうか

	bool m_isLastHitEventPlaying = false;//ラストヒットのイベント中かどうか


	std::unordered_map<AsyncData, int> m_asyncHandles; //非同期ロードのハンドルを保持するマップ


	//SoundManager m_soundManager;//サウンドマネージャー//GetSoundManager()経由でPlayBgm等を呼ぶ


};

