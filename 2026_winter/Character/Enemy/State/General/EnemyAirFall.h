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

private:
    float m_gravity = 0.0f;//このステート中の重力の累積
};
