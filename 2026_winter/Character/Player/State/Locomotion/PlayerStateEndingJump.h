#pragma once
#include "Character/Player/PlayerState.h"

//列車のエンディングで列車から飛び降りる(全部自動)//横へ跳んで、列車に置いていかれながら落ちる
class PlayerStateEndingJump :
    public PlayerState
{
public:
    //outDir:跳ぶ向き(水平)//backDir:列車の後ろの向き(置いていかれる向き)
    PlayerStateEndingJump(std::weak_ptr<Player> player, const Vector3& outDir, const Vector3& backDir);
    virtual ~PlayerStateEndingJump();

    void Enter() override;
    void Update() override;
    void Exit() override;

    bool IsInvincible()const override { return true; }

private:
    void SetPos(Player& player, const Vector3& pos);

private:
    Vector3 m_outDir;
    Vector3 m_backDir;
    Vector3 m_startPos;
    float m_frameCount = 0.0f;
    bool m_isFallAnim = false;
};
