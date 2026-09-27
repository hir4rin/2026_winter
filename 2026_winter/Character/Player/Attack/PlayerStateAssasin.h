#pragma once
#include "PlayerState.h"

class EnemyBase;

class PlayerStateAssasin :public PlayerState
{
public:
    PlayerStateAssasin(std::weak_ptr<Player> player);
    virtual ~PlayerStateAssasin();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;
private:
};

