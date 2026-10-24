#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"
#include "../../../CharacterBase.h"

/// <summary>
/// 大きくのけぞるHit(中身はEnemyHitGroundと同じで、アニメだけラージヒット)
/// </summary>
class EnemyHitLargeHit :
    public EnemyStateBase
{
public:
    EnemyHitLargeHit(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info);
    virtual ~EnemyHitLargeHit();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
    const char* GetName()const override { return "HitLargeHit"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Hit; }

private:
    CharacterBase::HitInfo m_info;//被弾情報
    float m_frame = 0.0f;//経過時間
};
