#pragma once
#include "../General/EnemyStateBase.h"
class EnemyAssasined :
    public EnemyStateBase
{
public:
    EnemyAssasined(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyAssasined();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;
private:

};

