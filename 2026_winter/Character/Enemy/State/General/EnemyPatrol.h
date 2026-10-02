#pragma once
#include "EnemyStateBase.h"
class EnemyPatrol :
    public EnemyStateBase
{
public:
    EnemyPatrol(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyPatrol();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();
private:
	float waitTimer = 0.0f;//待機時間のタイマー
};

