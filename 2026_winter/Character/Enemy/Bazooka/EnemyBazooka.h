#pragma once
#include "Character/Enemy/EnemyBase.h"
#include "Character/Enemy/EnemyPart.h"
#include <memory>
class EnemyBazookaWeapon;
class EnemyBazooka :
    public EnemyBase
{
public:
    EnemyBazooka(std::weak_ptr<Player> player,Vector3 startPos);
    virtual ~EnemyBazooka();

    void Init()override;
    void Update()override;
    void Draw()override;

    void OnCollision(Collider& other)override;
    void OnDamage(Collider& other, AttackData& data)override;

    void OnAssasined()override;
    void OnHeadBreak()override;
    void OnPartBreak()override;//攻撃で部位破壊したら左腕を落とす
    void SetOpacity(float rate)override;//本体に加えて、RPG7と切り離したパーツの不透明度も設定する
private:
    void SetUpBreakLeftArm();
    void SetUpBreakHead();

private:

    int m_leftArmModelHandle = -1;//部位破壊で落ちる左腕のモデル(盾なし)
    int m_headModelHandle = -1;//部位破壊で落ちる頭(ヘルメット)のモデル
    bool m_isBreakLeftArm = false;//左腕を部位破壊したか
    bool m_isBreakHead = false;//頭を部位破壊したか

    bool m_wasQPressed = false;//前のフレームにQを押していたか//デバッグ用の攻撃データ読み込み直しを押した瞬間だけ行うため

    std::shared_ptr<EnemyBazookaWeapon> m_weapon;//武器
    std::shared_ptr<EnemyPart> m_leftArmPart;//切り離された左腕(当たり判定・落下・回転を持つ)
    std::shared_ptr<EnemyPart> m_headPart;//切り離された頭(当たり判定・落下・回転を持つ)

    friend class EnemyBazookaWeapon;
};
