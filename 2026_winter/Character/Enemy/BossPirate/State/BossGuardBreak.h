#pragma once
#include "Character/Enemy/State/Defence/EnemyGuardBreak.h"

//第二ラウンドのボスのガードブレイク//背中の腕を切り離して打ち上げる
class BossGuardBreak :
    public EnemyGuardBreak
{
public:
    BossGuardBreak(std::weak_ptr<EnemyBase> owner);
    virtual ~BossGuardBreak();

    void Enter() override;
    void Exit()override;

    const char* GetName()const override { return "BossGuardBreak"; }
};
