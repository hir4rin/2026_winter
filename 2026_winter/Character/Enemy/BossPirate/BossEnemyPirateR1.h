#pragma once
#include "Character/Enemy/EnemyBase.h"
#include "Math/Matrix4x4.h"
#include "Character/Enemy/EnemyPart.h"
#include <memory>
class BossPirateWeapon;
class BossArm;
//第二ラウンドのボス(背中の腕あり)//第一ラウンドは BossEnemyPirateR0
//ステータス・攻撃・アニメのCSVは第二ラウンド用(BossEnemyPirateR1〜)
class BossEnemyPirateR1 :
    public EnemyBase
{
public:
    BossEnemyPirateR1(std::weak_ptr<Player> player,Vector3 startPos);
    virtual ~BossEnemyPirateR1();

    void Init()override;
    void Update()override;
    void Draw()override;
    bool IsChainAttackLookAtPlayer()const override { return true; }//連撃の2撃目でプレイヤーの方を向く
    bool HasDebugWindow()const override { return true; }//デバッグウィンドウを出す
    void DebugDraw()override;//腕の調整用のImGuiも出す

    std::vector<std::shared_ptr<EnemyBase>> CreateChildEnemies()override;//背中の腕を作る
    //背中の腕//まだ作っていない・消えたときは空
    std::shared_ptr<BossArm> GetLeftBackArm()const { return m_leftBackArm.lock(); }
    std::shared_ptr<BossArm> GetRightBackArm()const { return m_rightBackArm.lock(); }

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

    //背中の腕//EnemyManagerが持つ
    std::weak_ptr<BossArm> m_leftBackArm;
    std::weak_ptr<BossArm> m_rightBackArm;
};

