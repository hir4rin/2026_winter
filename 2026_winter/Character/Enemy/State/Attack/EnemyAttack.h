#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"
#include "Math/Vector3.h"
#include <memory>
class AttackCol;
class EnemyAttack :
    public EnemyStateBase
{
public:
    //isChain: 連撃の2撃目以降(AttackEndから来た)か
    EnemyAttack(std::weak_ptr<EnemyBase> owner, bool isChain = false);
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
    bool m_isChain = false;//連撃の2撃目以降か
    Vector3 m_moveDir;//踏み込む向き//Enterで決めて固定する(毎フレーム目標へ向け直すと、通り過ぎたときに前後に揺れるため)

};

