#pragma once
#include "../General/EnemyStateBase.h"
#include <memory>

//攻撃の後隙
//Chase->AttackPre->Attack->AttackEnd->(NextAfterAttackEnd、初期はBack)
//その場で止まり、向きも変えず、ガードもしない//プレイヤーが反撃できる時間
class EnemyAttackEnd :
    public EnemyStateBase
{
public:
    EnemyAttackEnd(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAttackEnd();

    void Enter() override;

    void Update()override;
    void Exit()override;

    const char* GetName()const override { return "AttackEnd"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Recovery; }
    std::string GetDebugInfo()const override;//経過フレーム/後隙の長さ

private:
    float m_timer = 0.0f;
};
