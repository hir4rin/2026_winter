#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"

//ミサイルのターン//中央へ移動→ミサイルを何回か撃つ→全部落ちるまで待つ→後隙
class KingmechMissileTurn :
    public EnemyStateBase
{
public:
    enum class Phase
    {
        Move,//中央へ移動
        Fire,//ミサイルを撃つ
        Wait,//ミサイルが全部落ちるまで待つ
        Recover,//後隙
    };

    KingmechMissileTurn(std::weak_ptr<EnemyBase> owner);
    virtual ~KingmechMissileTurn();

    void Enter() override;
    void Update()override;
    void Exit()override;

    const char* GetName()const override { return "KingmechMissileTurn"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Attack; }

private:
    void ChangePhase(Phase next);
    void FireAtPlayer();//プレイヤーの足元に1発撃つ

    Phase m_phase = Phase::Move;
    float m_frame = 0.0f;//今の段階の経過時間
    float m_fireTimer = 0.0f;//次に撃つまでの時間
    int m_shotCount = 0;//今のセットで撃った数
    int m_setCount = 0;//終わったセットの数
};
