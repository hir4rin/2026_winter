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
    const char* GetName()const override { return "Idle"; }
    std::string GetDebugInfo()const override;//経過フレーム/待機する長さ

};

