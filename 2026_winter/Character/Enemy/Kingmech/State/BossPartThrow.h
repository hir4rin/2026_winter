#pragma once
#include "Character/Enemy/State/EnemyStateBase.h"

//ガトリングを切り落とされてから、投げ返されて爆発するまで//膝をついて待ち、爆発したら立ち上がる
class BossPartThrow :
    public EnemyStateBase
{
public:
    BossPartThrow(std::weak_ptr<EnemyBase> owner);
    virtual ~BossPartThrow();

    void Enter() override;
    void Update()override;
    void Exit()override;

    void OnExploded();//ガトリングが当たって爆発した

    const char* GetName()const override { return "BossPartThrow"; }
    DebugCategory GetDebugCategory()const override { return DebugCategory::Hit; }

private:
    bool m_isExploded = false;//爆発して立ち上がっている
    float m_frame = 0.0f;//経過時間
};
