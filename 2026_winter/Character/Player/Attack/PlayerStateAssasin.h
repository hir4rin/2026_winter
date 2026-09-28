#pragma once
#include "PlayerState.h"

class EnemyBase;

class PlayerStateAssasin :public PlayerState
{
public:
    enum class AssasinState
    {
        Start,
        Execute,
        End,
    };
public:
    PlayerStateAssasin(std::weak_ptr<Player> player);
    virtual ~PlayerStateAssasin();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;
private:
    AssasinState m_state;
    float m_startTimer = 0.0f;
    float m_excuteTimer = 0.0f;
};

