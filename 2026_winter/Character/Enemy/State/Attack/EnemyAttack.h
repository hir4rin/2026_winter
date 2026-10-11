#pragma once
#include "../General/EnemyStateBase.h"
#include <memory>
class AttackCol;
class EnemyAttack :
    public EnemyStateBase
{
public:
    EnemyAttack(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAttack();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();
    const char* GetName()const override { return "Attack"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Attack; }
    std::string GetDebugInfo()const override;//アニメの進行率(判定が出ていれば「判定ON」)
private:
    void AttackMove();

    std::shared_ptr<AttackCol> m_attackCol;//攻撃の当たり判定

};

