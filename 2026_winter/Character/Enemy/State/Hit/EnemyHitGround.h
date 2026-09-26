#pragma once
#include "../General/EnemyStateBase.h"
#include "../../../CharacterBase.h"

/// <summary>
/// 地→地、空→空の際のHit
/// </summary>
class EnemyHitGround :
    public EnemyStateBase
{
public:
    EnemyHitGround(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info);
    virtual ~EnemyHitGround();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    CharacterBase::HitInfo m_info;//被弾情報
    float m_frame = 0.0f;//経過時間
};
