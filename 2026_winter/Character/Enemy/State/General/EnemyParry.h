#pragma once
#include "EnemyStateBase.h"
#include <memory>

//弾き//プレイヤーの攻撃を弾き返す
//終わったら NextAfterParry(初期はAttackPre)へ
//TODO:どこから入るか(弾く条件)は未定//案:ガード中に弱攻撃でゲージがたまったら EnemyGuard::OnGuardHit から入る
class EnemyParry :
    public EnemyStateBase
{
public:
    EnemyParry(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyParry();

    void Enter() override;

    void Update()override;
    void Exit()override;

    const char* GetName()const override { return "Parry"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Guard; }
    std::string GetDebugInfo()const override;//経過フレーム/弾きの長さ

private:
    float m_timer = 0.0f;
};
