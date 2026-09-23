#pragma once
class BattleManager
{
public:

	BattleManager();
	virtual ~BattleManager();

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
	void SetIsLastHitEventPlaying(bool ans) { m_isLastHitEventPlaying = ans; }
	bool GetIsLastHitEventPlaying() { return m_isLastHitEventPlaying; }
	//falseから初めてtrueになった瞬間だけ、内部でtrueにする(多重呼び出し防止)
	//void SetIsLastHitEventPlayingTrigger(bool ans);
private:
	bool m_isUltimating = false;//必殺技の演出中かどうか
	int m_ultCount = -1;//必殺技の演出時間カウント

	bool m_isPhotoMode = false;

	bool m_isEventPlaying = false;//イベント演出中かどうか

	bool m_isLastHitEventPlaying = false;//ラストヒットのイベント中かどうか

};

