#pragma once
#include "PlayerState.h"
class PlayerStateResultMove :
    public PlayerState
{
public:
    PlayerStateResultMove(std::weak_ptr<Player> player);
    virtual ~PlayerStateResultMove();

    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw()override;
private:
};

