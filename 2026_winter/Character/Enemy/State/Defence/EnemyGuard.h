#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"


class EnemyGuard :
    public EnemyStateBase
{
public:
    EnemyGuard(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyGuard();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
    const char* GetName()const override { return "Guard"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Guard; }
    std::string GetDebugInfo()const override;//ガードゲージと経過フレーム


    void OnGuardHit();
protected:
    virtual std::shared_ptr<EnemyStateBase> CreateGuardBreakState();//ガードブレイクのステートを作る

};

