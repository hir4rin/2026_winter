#pragma once
#include <memory>
#include "UIBase.h"

class Player;

//プレイヤーのHP・スキルゲージ・必殺技ゲージ
class PlayerHUD : public UIBase
{
public:
	PlayerHUD(std::weak_ptr<Player> player);
	~PlayerHUD() override;

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	//外枠と中身のバー(x,yは外枠の左上、設計解像度)
	void DrawBar(float x, float y, float scale, float rate, int fillHandle);
	//丸い必殺技ゲージ
	void DrawUlt();

	//今の値に表示を近づける
	float Follow(float disp, float target) const;

	std::weak_ptr<Player> m_player;

	//表示用の割合(0〜1)
	float m_dispHpRate = 1.0f;
	float m_dispSkillRate = 0.0f;
	float m_dispUltRate = 0.0f;

	int m_frame = 0;//満タンの点滅用

	int m_frameHandle = -1;//PlayerHP
	int m_hpHandle = -1;//gauge_hp
	int m_skillHandle = -1;//gauge_Skill
	int m_emptyHandle = -1;//gauge_brack
	int m_ultBaseHandle = -1;//UltGaugeBase
	int m_ultFrameHandle = -1;//UltGauge
	int m_ultColorHandle = -1;//UltGaugeColor
	int m_ultMaxHandle = -1;//UltGauge_circle
};
