#pragma once
#include "../General/EnemyStateBase.h"

class EnemyKnockBack :
    public EnemyStateBase
{
public:
    EnemyKnockBack(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyKnockBack();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    float m_downTime = 0.0f;//ダウンしている時間
};
