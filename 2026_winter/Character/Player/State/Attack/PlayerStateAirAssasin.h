#pragma once
#include "Character/Player/PlayerState.h"

class EnemyBase;

class PlayerStateAirAssasin :public PlayerState
{
public:
    enum class AssasinState
    {
        Approach,//接近
        Move,//敵と一緒に移動
        Execute,//頭破壊
        End,
    };
public:
    PlayerStateAirAssasin(std::weak_ptr<Player> player);
    virtual ~PlayerStateAirAssasin();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;
    bool IsInvincible()const override { return true; }//無敵
private:
    AssasinState m_state;
    float m_timer = 0.0f;//各段階の経過フレーム
};

