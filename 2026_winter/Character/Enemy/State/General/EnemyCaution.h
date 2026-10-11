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
    const char* GetName()const override { return "Caution"; }
    std::string GetDebugInfo()const override;//経過フレーム/警戒する長さ
};

