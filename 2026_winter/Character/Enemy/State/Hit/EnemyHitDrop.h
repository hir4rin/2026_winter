#pragma once
#include "../General/EnemyStateBase.h"
#include "../../../CharacterBase.h"

class EnemyHitDrop :
    public EnemyStateBase
{
public:
    EnemyHitDrop(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info);
    virtual ~EnemyHitDrop();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    CharacterBase::HitInfo m_info;//被弾情報
    float m_gravity = 0.0f;//このステート中の重力の累積
};
