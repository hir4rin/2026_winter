#pragma once
#include "EnemyStateBase.h"


class EnemyGuard :
    public EnemyStateBase
{
public:
    EnemyGuard(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyGuard();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

};

