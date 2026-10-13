#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"

class EnemyDie :
    public EnemyStateBase
{
public:
    EnemyDie(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyDie();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
    const char* GetName()const override { return "Die"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Dead; }
};
