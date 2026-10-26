#pragma once
#include "Character/Enemy/State/Defence/EnemyGuard.h"

//第二ラウンドのボスのガード//背中の腕もガードさせる
class BossGuard :
    public EnemyGuard
{
public:
    BossGuard(std::weak_ptr<EnemyBase> owner);
    virtual ~BossGuard();

    void Enter() override;
    void Exit()override;

    const char* GetName()const override { return "BossGuard"; }

protected:
    std::shared_ptr<EnemyStateBase> CreateGuardBreakState()override;//ボス用のガードブレイク
};
