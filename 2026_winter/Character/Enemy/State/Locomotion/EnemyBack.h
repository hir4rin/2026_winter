#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"
class EnemyBack :
    public EnemyStateBase
{
public:
    EnemyBack(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyBack();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();
    const char* GetName()const override { return "Back"; }
};

