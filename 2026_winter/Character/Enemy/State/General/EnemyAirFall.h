#pragma once
#include "EnemyStateBase.h"

class EnemyAirFall :
    public EnemyStateBase
{
public:
    EnemyAirFall(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAirFall();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
    const char* GetName()const override { return "AirFall"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Hit; }

private:
    float m_gravity = 0.0f;//このステート中の重力の累積
};
