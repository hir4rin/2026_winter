#pragma once
#include "PlayerState.h"
#include "PlayerStateDodge.h"

//ジャスト回避成功後の状態//PlayerStateDodgeでジャスト回避が成功したら遷移する
class PlayerStateJustDodge : public PlayerState
{
public:
    PlayerStateJustDodge(std::weak_ptr<Player> player, PlayerStateDodge::AvoidState avoidState);
    virtual ~PlayerStateJustDodge();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;

    bool IsInvincible()const override { return true; }//ジャスト回避中はずっと無敵
private:
    PlayerStateDodge::AvoidState m_avoidState;//直前の回避が前回避か後ろ回避か//アニメーションの切り替えに使う
};

