#pragma once
#include <memory>
#include "DxLib.h"
#include "UI/UIBase.h"

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
	//道に沿った位置と向き(設計座標)
	struct PathPoint
	{
		float x, y;
		float nx, ny;//外向き
	};
	PathPoint GetPathPoint(float rate) const;

	//設計座標 → 画面
	VECTOR ToScreen(float x, float y) const;

	void UpdateTassel();

	void DrawUlt();
	void DrawHp();
	void DrawSkill();
	void DrawTassel();

	//シートの画像を道の0〜rateの所だけ描く
	void DrawHpRange(int handle, float from, float to, COLOR_U8 color);
	//スキルの帯のfrom〜toの所だけ描く
	void DrawSkillRange(int handle, float from, float to, COLOR_U8 color);
	void DrawSheetPolygon(const float (*pos)[2], int num, int handle, COLOR_U8 color);

	//今の値に表示を近づける
	float Follow(float disp, float target, float rate) const;

	std::weak_ptr<Player> m_player;

	//表示用の割合(0〜1)
	float m_dispHpRate = 1.0f;
	float m_trailRate = 1.0f;//減った分の残像
	int m_trailWait = 0;
	float m_dispSkill = 0.0f;//スキル(1回分=1)
	float m_dispUltRate = 0.0f;

	int m_lastHp = 0;
	int m_lastSkill = 0;

	float m_readyFlash = 0.0f;//1回分たまった瞬間
	float m_useFlash = 0.0f;//使った瞬間
	float m_useFrom = 0.0f;//使った所(帯の割合)
	float m_useTo = 0.0f;

	//房の揺れ(度)
	float m_swing = 0.0f;
	float m_swingVel = 0.0f;
	float m_tipSwing = 0.0f;//毛先(遅れてついてくる)
	float m_tipSwingVel = 0.0f;

	int m_frame = 0;

	int m_frameSheet = -1;//hud_frame
	int m_fillSheet = -1;//hud_fill
	int m_bandSheet = -1;//hud_band
	int m_tasselHandle = -1;//hud_tassel
	int m_ultBaseHandle = -1;//UltGaugeBase
	int m_ultFrameHandle = -1;//UltGauge
	int m_ultColorHandle = -1;//UltGaugeColor
	int m_ultMaxHandle = -1;//UltGauge_circle
	int m_ultGlowHandle = -1;//hud_ultglow
	int m_ultSparkHandle = -1;//hud_spark
	int m_markOnHandle = -1;//hud_markon
	int m_markOffHandle = -1;//hud_markoff
};
