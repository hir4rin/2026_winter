#pragma once
#include "EnemyBase.h"
class EnemySwordman :
    public EnemyBase
{
public:
    EnemySwordman(std::weak_ptr<Player> player,Vector3 startPos);
    virtual ~EnemySwordman();

    void Init()override;
    void Update()override;
    void Draw()override;

    void OnCollision(Collider& other)override;
    void OnDamage(Collider& other, AttackData& data)override;
    
private:

    int m_hitEfHandle = -1;//ヒットエフェクトのハンドル
    int m_hitEfPlayingHandle = -1;//再生中のヒットエフェクトのハンドル
};

