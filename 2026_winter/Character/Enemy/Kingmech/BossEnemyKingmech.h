#pragma once
#include "Character/Enemy/EnemyBase.h"
#include "Math/Matrix4x4.h"
#include <memory>

class KingmechGatling;

//新しいボス(Kingmech)//中身は BossEnemyPirateR0 と同じ(剣・部位破壊で落ちるパーツは無し)
//ステータス・攻撃・アニメのCSVは BossEnemyKingmech〜 を使う
class BossEnemyKingmech :
    public EnemyBase
{
public:
    BossEnemyKingmech(std::weak_ptr<Player> player,Vector3 startPos);
    virtual ~BossEnemyKingmech();

    void Init()override;
    void Update()override;
    void Draw()override;
    bool IsChainAttackLookAtPlayer()const override { return true; }//連撃の2撃目でプレイヤーの方を向く
    bool HasDebugWindow()const override { return true; }//デバッグウィンドウを出す

    void OnCollision(Collider& other)override;
    void OnDamage(Collider& other, AttackData& data)override;

    void OnAssasined()override;
    void OnHeadBreak()override;//落とすパーツが無いのでフラグだけ
    void OnPartBreak()override;//落とすパーツが無いのでフラグだけ

    void ThrowPart()override;//ガトリングを投げ返す
protected:
    int SelectAttackIndex()override;//出す攻撃を選ぶ(無ければ-1)//ボス用の分岐

    //ステートの切り替え//今は EnemyBase と同じ
    std::shared_ptr<EnemyStateBase> NextAfterIdle()override;//Idleの待機時間が終わった
    std::shared_ptr<EnemyStateBase> NextOnArrived()override;//Chaseで攻撃距離に入った、または目的地に着いた
    std::shared_ptr<EnemyStateBase> NextAfterCaution()override;//Cautionの警戒時間が終わった
    std::shared_ptr<EnemyStateBase> NextAfterAttack()override;//攻撃のモーションが終わった
    std::shared_ptr<EnemyStateBase> NextAfterAttackEnd()override;//攻撃の後隙が終わった//攻撃の回数を数える
    std::shared_ptr<EnemyStateBase> NextAfterParry()override;//弾きが終わった
    std::shared_ptr<EnemyStateBase> NextOnPlayerFound()override;//プレイヤーを見つけた
    std::shared_ptr<EnemyStateBase> NextAfterRecover()override;//被弾・ダウン・ガードなどから立ち直った
private:
    void CutGatling(Collider& other, AttackData& data);//切り上げでガトリングを切り落とす
    void OnGatlingExploded();//投げ返されたガトリングが爆発した//ガトリングから呼ばれる

    std::shared_ptr<KingmechGatling> m_gatling;//右腕のガトリング

    int m_attackCountSinceLarge = 0;//前の全体攻撃から何回攻撃したか

    bool m_wasQPressed = false;//前のフレームにQを押していたか//デバッグ用の攻撃データ読み込み直しを押した瞬間だけ行うため
    bool m_wasIPressed = false;//前のフレームにIを押していたか//デバッグ用のアニメCSV読み込み直し

    friend class BossPartThrow;
    friend class KingmechGatling;
};

