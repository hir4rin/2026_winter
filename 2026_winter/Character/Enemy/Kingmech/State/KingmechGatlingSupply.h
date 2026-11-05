#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"

//ガトリングの補給//呼ぶ→落ちてくるのを待つ→後隙
class KingmechGatlingSupply :
    public EnemyStateBase
{
public:
    enum class Phase
    {
        Call,//補給を呼ぶ
        Drop,//落ちてくるのを待つ
        Recover,//後隙
    };

    KingmechGatlingSupply(std::weak_ptr<EnemyBase> owner);
    virtual ~KingmechGatlingSupply();

    void Enter() override;
    void Update()override;
    void Exit()override;

    const char* GetName()const override { return "KingmechGatlingSupply"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Attack; }

private:
    void ChangePhase(Phase next);

    Phase m_phase = Phase::Call;
    float m_frame = 0.0f;//今の段階の経過時間
};
