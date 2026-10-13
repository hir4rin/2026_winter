#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"


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

    //他のステートに移行していいか
    bool CanChangeHit();

    void DebugDraw()override;
    const char* GetName()const override { return "GuardBreak"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Guard; }

private:
    GuardBreakState m_state = GuardBreakState::Start;
    float m_middleTimer = 0.0f;//Middle(ダウン中)の経過時間
};
