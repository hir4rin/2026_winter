#pragma once
#include <memory>
#include "UI/UIBase.h"

class EnemyManager;
class EnemyBase;

//ボスのHPバー(画面の下の真ん中)
class BossHpUI : public UIBase
{
public:
	BossHpUI(std::weak_ptr<EnemyManager> enemyManager);
	~BossHpUI() override;

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	enum class State
	{
		Hidden,//ボスがいない
		Intro,//名前とバーが出てくる
		Active,//ふつう
		Break,//HPが0になって斬られて消える
	};

	//ボスを探す(いなければ空)
	std::shared_ptr<EnemyBase> FindBoss() const;
	void ChangeState(State state);
	//表示を隠すか(イベント・トドメ・エンディング中)
	bool IsHideScene() const;

	//バーを描く(dy:上下にずらす量、alpha:不透明度)
	void DrawBar(float dy, int alpha) const;
	void DrawName(int alpha) const;
	void DrawBreak() const;

	std::weak_ptr<EnemyManager> m_enemyManager;
	std::weak_ptr<EnemyBase> m_boss;

	State m_state = State::Hidden;
	int m_frame = 0;//今の状態になってからのフレーム

	float m_hpRate = 1.0f;//今のHPの割合
	float m_whiteRate = 1.0f;//遅れて減る白い所
	int m_whiteWait = 0;//白い所が減り始めるまで
	int m_blinkFrame = 0;//残り少ないときの点滅用

	int m_fontHandle = -1;
};
