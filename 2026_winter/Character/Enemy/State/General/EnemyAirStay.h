#pragma once
#include "EnemyStateBase.h"

class EnemyAirStay :
    public EnemyStateBase
{
public:
    EnemyAirStay(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAirStay();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    float m_airTime = 0.0f;//空中で浮いている時間
};
