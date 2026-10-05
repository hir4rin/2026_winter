#pragma once
#include "PlayerState.h"

class Input;
class JustDodgeCol;


class PlayerStateDodge : public PlayerState
{
public:
    enum class AvoidState
    {
        Forward,
        Backward
    };
public:
    PlayerStateDodge(std::weak_ptr<Player> player);
    virtual ~PlayerStateDodge();
    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw() override;

    void OnJustDodge(Collider& other, const CharacterBase::AttackData& data) override;//ジャスト回避成功時の処理
    bool IsInvincible()const override;//ジャスト回避の受付中は無敵
private:
    void ReleaseJustDodgeCol();//ジャスト回避判定を削除する
private:
    AvoidState m_avoidState;

    std::shared_ptr<JustDodgeCol> m_justDodgeCol;//ジャスト回避判定//回避中だけ存在する
    float m_justDodgeTimer = 0.0f;//回避開始からの経過フレーム//ジャスト回避の受付時間の管理に使う
    bool m_isJustDodged = false;//ジャスト回避が成功したかどうか//1回の回避で1回だけ成功させる
};

