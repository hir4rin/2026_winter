#pragma once
#include "PlayerState.h"
class PlayerStateIdle :
    public PlayerState
{
public:
    PlayerStateIdle(std::weak_ptr<Player> player);
	virtual ~PlayerStateIdle();
	void Enter() override;
	void Update() override;
	void Exit() override;

	void DebugDraw()override;
};

