#pragma once
#include "../General/EnemyStateBase.h"
#include "../../../CharacterBase.h"



/// <summary>
/// 地上から空中へと遷移する攻撃
/// </summary>
class EnemyHitAir :
    public EnemyStateBase
{
public:
    EnemyHitAir(std::weak_ptr<EnemyBase> owner, const CharacterBase::HitInfo& info);
    virtual ~EnemyHitAir();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    CharacterBase::HitInfo m_info;//被弾情報
};

