#pragma once
#include "EnemyStateBase.h"


class EnemyIdle :
    public EnemyStateBase
{
public:
    EnemyIdle(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyIdle();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();

};

