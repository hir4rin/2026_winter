#pragma once
#include "EnemyBase.h"
#include "../../Math/Matrix4x4.h"
#include "EnemyPart.h"
#include <memory>
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

    void OnAssasined()override;
private:
    void SetUpBreakLeftArm();
    void SetUpBreakHead();
    
private:

    int m_hitEfHandle = -1;//ヒットエフェクトのハンドル
    int m_hitEfPlayingHandle = -1;//再生中のヒットエフェクトのハンドル

    int m_leftArmModelHandle = -1;//部位破壊で落ちる左腕のモデル
    int m_headModelHandle = -1;//部位破壊で落ちる頭(ヘルメット)のモデル
    bool m_isBreakLeftArm = false;//左腕を部位破壊したか
    bool m_isBreakHead = false;//頭を部位破壊したか

    std::shared_ptr<EnemyPart> m_leftArmPart;//切り離された左腕(当たり判定・落下・回転を持つ)
    std::shared_ptr<EnemyPart> m_headPart;//切り離された頭(当たり判定・落下・回転を持つ)
};

