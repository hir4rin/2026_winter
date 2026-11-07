#pragma once
#include <memory>
#include "UIBase.h"

class Player;
class EnemyBase;

//ガードブレイクのチャンス中に敵の体の中心に出すUI
class GuardBreakChanceUI : public UIBase
{
public:
	GuardBreakChanceUI(std::weak_ptr<Player> player);
	~GuardBreakChanceUI() override;

	void Init()override;
	void Update()override;
	void Draw()override;

private:
	//UIを消して最初の状態に戻す
	void Hide();

	float GetBladeOpenRate(float delay) const;//刃の伸び具合(0〜1)
	float GetAlpha() const;//全体の透明度(0〜1)
	bool IsWarning() const;//残りわずかで光っている

	//x,yは×の中心(実解像度)
	void DrawGlow(float x, float y, float scale);
	void DrawBlades(float x, float y, float scale);
	void DrawBladePair(int handle, float x, float y, float length, float thick);
	void DrawBlade(int handle, float x, float y, float angle, float openRate, float length, float thick);
	void DrawBand(float x, float y, float scale);
	void DrawLabel(float x, float y, float scale);
	void DrawCenter(float x, float y, float scale);
	void DrawRing(float x, float y, float radius, float rate, float thick, unsigned int color);
	void DrawRipple(float x, float y, float scale);

	std::weak_ptr<Player> m_player;
	std::weak_ptr<EnemyBase> m_target;

	int m_frame = 0;//出てからのフレーム
	float m_remainRate = 1.0f;//チャンスの残り(0〜1)

	bool m_isExiting = false;//打ち上げのリアクション中
	int m_exitFrame = 0;
	bool m_isFading = false;//時間切れで消えている
	int m_fadeFrame = 0;
	bool m_isOnScreen = false;//前のDrawで画面に出せた
	float m_drawX = 0.0f;//×の中心
	float m_drawY = 0.0f;

	int m_bladeHandle = -1;//刃
	int m_bladeGlowHandle = -1;//刃の周りの光
	int m_whiteBladeHandle = -1;//白い刃(光らせる用)
	int m_glowHandle = -1;//後ろの光
	int m_discHandle = -1;//真ん中の丸
	int m_buttonHandle = -1;//Yの丸
	int m_bandHandle = -1;//帯
	int m_labelFontHandle = -1;//「ガードブレイク」の文字
	int m_buttonFontHandle = -1;//Yの文字
};
