#pragma once
#include <memory>
#include "UI/UIBase.h"

class Player;
class EnemyBase;

//ロックオン中の敵に出すレティクル(ツクモの走査)
class LockOnReticleUI : public UIBase
{
public:
	LockOnReticleUI(std::weak_ptr<Player> player);

	void Init()override;
	void Update()override;
	void Draw()override;

private:
	std::shared_ptr<EnemyBase> FindTarget() const;//出さないときはnullptr
	void Hide();

	float GetInRate() const;//入りの進み(0〜1)
	float GetAlpha() const;
	float GetSizeScale(const std::shared_ptr<EnemyBase>& target) const;//大きい敵ほど大きく

	//x,yは中心(実解像度)
	void DrawArcs(float x, float y, float scale, float alpha);
	void DrawDots(float x, float y, float scale, float alpha);
	void DrawScanLine(float x, float y, float scale, float alpha);
	void DrawLockBox(float x, float y, float scale, float alpha);
	void DrawArc(float x, float y, float radius, float startAngle, float angleRange, float thick, unsigned int color);

	std::weak_ptr<Player> m_player;
	std::weak_ptr<EnemyBase> m_target;

	int m_frame = 0;//ロックしてからのフレーム
	int m_totalFrame = 0;//回転・走査用
	bool m_isFading = false;//外れて消えている
	int m_fadeFrame = 0;
	bool m_isOnScreen = false;//前のDrawで画面に出せた
	float m_drawX = 0.0f;
	float m_drawY = 0.0f;
	float m_drawScale = 1.0f;
};
