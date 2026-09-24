#pragma once
#include "../General/EnemyStateBase.h"
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
private:
    void AttackMove();

    //ここで当たり判定を持つ

};

