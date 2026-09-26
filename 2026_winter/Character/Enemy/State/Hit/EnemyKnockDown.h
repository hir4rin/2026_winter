#pragma once
#include "../General/EnemyStateBase.h"



//今はまだ使っていない、後々ボスのノックダウン状態などに使う可能性高い

class EnemyKnockDown :
    public EnemyStateBase
{
public:
    EnemyKnockDown(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyKnockDown();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    float m_downTime = 0.0f;//ダウンしている時間
};
