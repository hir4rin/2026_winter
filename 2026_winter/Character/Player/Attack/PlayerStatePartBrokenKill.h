#pragma once
#include "PlayerState.h"
class PlayerStatePartBrokenKill :
    public PlayerState
{
public:
    enum class PartBrokenKill
    {
        Start,
        Execute,
        End,
    };
public:
    PlayerStatePartBrokenKill(std::weak_ptr<Player> player);
    virtual ~PlayerStatePartBrokenKill();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;
private:
    PartBrokenKill m_state;
    float m_startTimer = 0.0f;
    float m_excuteTimer = 0.0f;
};

