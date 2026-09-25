#pragma once
#include "../General/EnemyStateBase.h"

class EnemyKnockDown :
    public EnemyStateBase
{
public:
    EnemyKnockDown(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyKnockDown();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    float m_downTime = 0.0f;//ダウンしている時間
};
