#pragma once
#include "Character/Enemy/EnemyBase.h"
#include "Math/Matrix4x4.h"
#include "Character/Enemy/EnemyPart.h"
#include <memory>
class BossPirateWeapon;
//第一ラウンドのボス(背中の腕なし)//第二ラウンドは BossEnemyPirateR1
//ステータス・攻撃・アニメのCSVは BossEnemyPirate〜 を使う(第二ラウンドは BossEnemyPirateR1〜)
class BossEnemyPirateR0 :
    public EnemyBase
{
public:
    BossEnemyPirateR0(std::weak_ptr<Player> player,Vector3 startPos);
    virtual ~BossEnemyPirateR0();

    void Init()override;
    void Update()override;
    void Draw()override;
    bool IsChainAttackLookAtPlayer()const override { return true; }//連撃の2撃目でプレイヤーの方を向く
    bool HasDebugWindow()const override { return true; }//デバッグウィンドウを出す

    void OnCollision(Collider& other)override;
    void OnDamage(Collider& other, AttackData& data)override;

    void OnAssasined()override;
    void OnHeadBreak()override;
    void OnPartBreak()override;//攻撃で部位破壊したら左腕を落とす
    void SetOpacity(float rate)override;//本体に加えて、切り離したパーツの不透明度も設定する
protected:
    int SelectAttackIndex()override;//出す攻撃を選ぶ(無ければ-1)//ボス用の分岐
    std::shared_ptr<EnemyStateBase> NextAfterAttackEnd()override;//攻撃の回数を数える
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

    int m_attackCountSinceLarge = 0;//前の全体攻撃から何回攻撃したか

    bool m_wasQPressed = false;//前のフレームにQを押していたか//デバッグ用の攻撃データ読み込み直しを押した瞬間だけ行うため
    bool m_wasIPressed = false;//前のフレームにIを押していたか//デバッグ用のアニメCSV読み込み直し

    std::shared_ptr<EnemyPart> m_leftArmPart;//切り離された左腕(当たり判定・落下・回転を持つ)
    std::shared_ptr<EnemyPart> m_headPart;//切り離された頭(当たり判定・落下・回転を持つ)

    std::shared_ptr<BossPirateWeapon> m_weapon;//右手の剣
};

