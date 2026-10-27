#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"
#include "Math/Vector3.h"

//切り離した腕が付け根に戻る//着いたらIdle
class BossArmReturn :
    public EnemyStateBase
{
public:
    BossArmReturn(std::weak_ptr<EnemyBase> owner);
    virtual ~BossArmReturn();

    void Enter() override;
    void Update()override;
    void Exit()override;

    const char* GetName()const override { return "BossArmReturn"; }

private:
    float m_timer = 0.0f;//経過フレーム
    Vector3 m_startMove;//戻り始めの移動
    float m_startSpinDeg = 0.0f;//戻り始めの回転
    float m_goalSpinDeg = 0.0f;//戻り終わりの回転//一番近い一周分
};
