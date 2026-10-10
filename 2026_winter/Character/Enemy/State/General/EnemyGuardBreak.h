#pragma once
#include "EnemyStateBase.h"


class EnemyGuardBreak :
    public EnemyStateBase
{
public:
    enum class GuardBreakState
    {
        Start,
        Middle,
        End,
    };
public:
    EnemyGuardBreak(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyGuardBreak();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    GuardBreakState m_state = GuardBreakState::Start;
    float m_middleTimer = 0.0f;//Middle(ダウン中)の経過時間
};
