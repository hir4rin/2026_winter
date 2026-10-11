#pragma once
#include "PlayerState.h"

//攻撃を敵に弾かれてひるんでいる状態
//EnemyParryから Player::OnParried で入る
//一定時間たったら、床にいればIdle/Move
class PlayerAttackParried :
    public PlayerState
{
public:
    PlayerAttackParried(std::weak_ptr<Player> player);
    virtual ~PlayerAttackParried();
    void Enter() override;
    void Update() override;
    void Exit() override;
    void DebugDraw()override;

private:
    float m_timer = 0.0f;

    Vector3 m_moveVec = Vector3();
};
