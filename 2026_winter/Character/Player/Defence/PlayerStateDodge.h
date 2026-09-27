#pragma once
#include "PlayerState.h"

class Input;


class PlayerStateDodge : public PlayerState
{
public:
    enum class AvoidState
    {
        Forward,
        Backward
    };
public:
    PlayerStateDodge(std::weak_ptr<Player> player);
    virtual ~PlayerStateDodge();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;
private:
    AvoidState m_avoidState;
};

