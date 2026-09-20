#pragma once
#include "PlayerState.h"

class PlayerStateDashAttack : public PlayerState
{
public:
	PlayerStateDashAttack(std::weak_ptr<Player> player);
	virtual ~PlayerStateDashAttack();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;

private:
	void AttackMove();//突進攻撃の移動処理
};

