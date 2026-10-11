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
    const char* GetName()const override { return "KnockBack"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Hit; }

private:
    float m_downTime = 0.0f;//ダウンしている時間
};
