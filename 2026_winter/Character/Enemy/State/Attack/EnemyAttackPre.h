#pragma once
#include "../General/EnemyStateBase.h"
#include <memory>

//攻撃の予備動作
//Chase->AttackPre->Attack
class EnemyAttackPre :
    public EnemyStateBase
{
public:
    EnemyAttackPre(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAttackPre();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();
    const char* GetName()const override { return "AttackPre"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::AttackPre; }
    std::string GetDebugInfo()const override;//経過フレーム/予備動作の長さ(向きを固定したら「固定」)

private:
    float m_timer = 0.0f;

};
