#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"

//ボスから切り離された腕//打ち上がって浮く//戻すのはボス(RequestReturn)
class BossArmDetached :
    public EnemyStateBase
{
public:
    BossArmDetached(std::weak_ptr<EnemyBase> owner);
    virtual ~BossArmDetached();

    void Enter() override;
    void Update()override;
    void Exit()override;

    const char* GetName()const override { return "BossArmDetached"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Hit; }

private:
    float m_timer = 0.0f;//経過フレーム
};
