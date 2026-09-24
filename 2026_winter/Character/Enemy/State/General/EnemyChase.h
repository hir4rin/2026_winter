#pragma once
#include "EnemyStateBase.h"
class EnemyChase :
    public EnemyStateBase
{
public:
    EnemyChase(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyChase();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();
};

