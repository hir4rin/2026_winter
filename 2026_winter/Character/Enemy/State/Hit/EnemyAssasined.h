#pragma once
#include "../General/EnemyStateBase.h"
class EnemyAssasined :
    public EnemyStateBase
{
public:
    enum class AssasinState
    {
        Start,
        Execute,
        End,
    };
public:
    EnemyAssasined(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAssasined();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
private:
    AssasinState m_state;
    float m_startTimer = 0.0f;
    float m_excuteTimer = 0.0f;
};

