#pragma once
#include "../General/EnemyStateBase.h"
#include "../../../CharacterBase.h"

//打ち上げられた時の状態//上昇しきったらAirStayへ
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
