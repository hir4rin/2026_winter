#pragma once
#include "../General/EnemyStateBase.h"
class EnemyPartBrokenKilled :
    public EnemyStateBase
{
public:
    enum class PartBrokenKill
    {
        Start,
        Execute,
        End,
    };
public:
    EnemyPartBrokenKilled(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyPartBrokenKilled();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
private:
    PartBrokenKill m_state;
    float m_startTimer = 0.0f;
    float m_excuteTimer = 0.0f;
};

