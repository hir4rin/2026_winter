#pragma once
#include "../General/EnemyStateBase.h"

//死体を少しずつ透明にして消すState//消え終わったらEnemyManagerが削除する
class EnemyVanish :
    public EnemyStateBase
{
public:
    EnemyVanish(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyVanish();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw()override;

private:
    float m_timer = 0.0f;//消え始めてからのフレーム数
};
