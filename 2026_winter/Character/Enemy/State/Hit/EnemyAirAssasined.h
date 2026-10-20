#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"
class EnemyAirAssasined :
    public EnemyStateBase
{
public:
    enum class AssasinState
    {
        Approach,//接近
        Move,//プレイヤーと一緒に移動
        Execute,//頭破壊
        End,
    };
public:
    EnemyAirAssasined(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAirAssasined();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
    const char* GetName()const override { return "AirAssasined"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Dead; }
private:
    AssasinState m_state;
    float m_timer = 0.0f;//各段階の経過フレーム
};

