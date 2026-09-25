#pragma once
#include "../General/EnemyStateBase.h"

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
};
