#pragma once
#include <memory>
#include "Math/Vector3.h"

class LockOnManager;
class AttackTokenManager;
class BloodDecalManager;
class ResultRecorder;

class BattleManager
{
public:
	//トドメのムービーの画面効果
	struct FinisherScreen
	{
		bool isPlaying = false;
		float whiteRate = 0.0f;//白いフラッシュ
		float slashRate = 0.0f;//斬撃の線
	};

	BattleManager();
	virtual ~BattleManager();

	void Init();
	void Update();

	//burstPos:白い爆発を出すワールド座標
	void SetUltStart(int frames, const Vector3& burstPos) { m_ultCount = frames; m_isUltimating = true; m_ultBurstPos = burstPos; };
	const Vector3& GetUltBurstPos()const { return m_ultBurstPos; }
	bool GetIsUltimating() { return m_isUltimating; };
	void SetUltEnd() { m_ultCount = -1; m_isUltimating = false; };

	void SetPhotoMode(bool ans) { m_isPhotoMode = ans; }
	bool GetPhotoMode() { return m_isPhotoMode; }
	//ステージ編集モード(壁ゾーンの配置・編集)。フォトモードと同じフリーカメラを使う
	void SetStageEditMode(bool ans) { m_isStageEditMode = ans; }
	bool GetStageEditMode() { return m_isStageEditMode; }

	//イベント演出中(カメラ演出+その戻りのBlend中)はプレイヤー/敵の入力・行動を止めるためのフラグ
	void SetIsEventPlaying(bool ans) { m_isEventPlaying = ans; }
	bool GetIsEventPlaying() { return m_isEventPlaying; }

	//ラストヒットのイベント中かどうか
	void SetIsLastHitEventPlaying(bool ans) { m_isLastHitEventPlaying = ans; }
	bool GetIsLastHitEventPlaying() { return m_isLastHitEventPlaying; }
	//falseから初めてtrueになった瞬間だけ、内部でtrueにする(多重呼び出し防止)
	//void SetIsLastHitEventPlayingTrigger(bool ans);

	//全フェーズが終わったか(シーン遷移の合図)
	void SetIsBattleEnd(bool ans) { m_isBattleEnd = ans; }
	bool GetIsBattleEnd() { return m_isBattleEnd; }

	std::weak_ptr<AttackTokenManager> GetAttackTokenMgr() { return m_attackTokenMgr; }
	std::weak_ptr<BloodDecalManager> GetBloodDecalMgr() { return m_bloodDecalMgr; }
	std::weak_ptr<ResultRecorder> GetResultRecorder() { return m_resultRecorder; }
	FinisherScreen& GetFinisherScreen() { return m_finisherScreen; }

	//画面に血を付ける(暗殺・確殺。ScreenBloodUIが受け取る)
	void RequestScreenBlood() { m_isScreenBloodRequested = true; }
	//来ていればtrue(受け取ったら消える)
	bool TakeScreenBloodRequest();
private:
	std::shared_ptr<AttackTokenManager> m_attackTokenMgr;//敵の攻撃権
	std::shared_ptr<BloodDecalManager> m_bloodDecalMgr;//床の血の跡
	std::shared_ptr<ResultRecorder> m_resultRecorder;//リザルト用の記録

	bool m_isUltimating = false;//必殺技の演出中かどうか
	int m_ultCount = -1;//必殺技の演出時間カウント
	Vector3 m_ultBurstPos;//白い爆発を出す位置

	bool m_isPhotoMode = false;
	bool m_isStageEditMode = false;//ステージ編集モード中かどうか

	bool m_isEventPlaying = false;//イベント演出中かどうか

	bool m_isLastHitEventPlaying = false;//ラストヒットのイベント中かどうか

	bool m_isBattleEnd = false;//バトルが終わったか
	FinisherScreen m_finisherScreen;
	bool m_isScreenBloodRequested = false;//画面の血

};

