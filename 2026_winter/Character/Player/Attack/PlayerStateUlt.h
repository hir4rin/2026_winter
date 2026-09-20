#pragma once
#include "PlayerState.h"

class AttackCol;

class PlayerStateUlt : public PlayerState
{
public:
	PlayerStateUlt(std::weak_ptr<Player> player);
	virtual ~PlayerStateUlt();

	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;
private:
	void DetermineAttackDirection();//攻撃の方向を決める関数
	void EffectCheck();//エフェクトが発生したかどうか、位置、回転の再設定
private:
	std::shared_ptr<AttackCol> m_attackCol;//攻撃の当たり判定
	bool m_isTriggerdEffec = false;//エフェクトを発生させたかどうか
};

