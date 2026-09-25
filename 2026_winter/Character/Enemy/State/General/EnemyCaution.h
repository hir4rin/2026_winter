#pragma once
#include "EnemyStateBase.h"
class EnemyCaution :
    public EnemyStateBase
{
public:
    EnemyCaution(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyCaution();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();
};

