# 敵のステート遷移のまとめ

敵の「次に何をするか」を決める遷移は、`EnemyBase` の仮想関数(6個)にまとめてある。
敵の種類ごとに行動を変えたいときは、派生クラス(`EnemySwordman` など)でこの関数を上書きする。
初期の遷移先は、仮想関数にする前(2026-10-11)の各Stateの遷移と同じなので、上書きしなければ動きは変わらない。

## 判断の遷移(敵ごとに上書きできる)

| 関数 | いつ呼ばれるか | 呼び出し元 | 初期の遷移先 |
|---|---|---|---|
| `NextAfterIdle()` | Idleの待機時間(120F)が終わった | `EnemyIdle::Update` | 攻撃できれば AttackPre、できなければ Chase / Caution をランダム |
| `NextOnArrived()` | Chaseで攻撃距離に入った、または目的地に着いた | `EnemyChase::Update`(2か所) | 攻撃できれば AttackPre、できなければ Caution |
| `NextAfterCaution()` | Cautionの警戒時間(60F)が終わった | `EnemyCaution::Update` | Chase |
| `NextAfterAttack()` | 攻撃のモーションが終わった | `EnemyAttack::Update` | AttackEnd(後隙) |
| `NextAfterAttackEnd()` | 攻撃の後隙が終わった | `EnemyAttackEnd::Update` | Back |
| `NextAfterParry()` | 弾きが終わった | `EnemyParry::Update` | AttackPre(反撃) |
| `NextOnPlayerFound()` | 巡回・見張り中にプレイヤーを見つけた | `EnemyPatrol::Update`、`EnemyBase::OnAlerted`(グループの仲間が見つけたとき) | Chase |
| `NextAfterRecover()` | 被弾・ダウン・着地・ガード・ガードブレイクから立ち直った | `EnemyHitGround`(床にいるとき)、`EnemyKnockDown`、`EnemyKnockBack`、`EnemyAirFall`(着地)、`EnemyGuard`(時間切れ)、`EnemyGuardBreak`(起き上がり後) | Idle |

- 「攻撃できる」= `SelectAttackIndex() >= 0`(クールタイムが0以下 かつ 距離が attackRange 以内の攻撃がある)。
  選んだ index は `NextAfterIdle` / `NextOnArrived` で `m_currentAttackIndex` に入れてから AttackPre へ。`NextAfterParry` は index 0 固定(仮)。
- `NextOnArrived` は Chase の中で毎フレーム呼ばれる可能性がある。Chase を返すように上書きすると、毎フレーム Chase に入り直すので注意。

## 流れの遷移(各Stateの中に直接書いたまま)

どの敵でも同じになる「物理・モーションの続き」なので、仮想関数にしていない。

| 遷移 | 場所 |
|---|---|
| AttackPre → Attack | `EnemyAttackPre::Update` |
| HitAir → AirStay → AirFall | `EnemyHitAir` / `EnemyAirStay` |
| HitGround → AirStay(空中で終わったとき) | `EnemyHitGround::Update` |
| HitDrop → KnockDown / Die(死亡予定のとき) | `EnemyHitDrop::Update` |
| Guard → GuardBreak(ゲージがたまった) | `EnemyGuard::OnGuardHit` |
| Back → Idle | `EnemyBack::Update`(Backの後に何をするかは NextAfterIdle で決まる) |
| 被弾 → Hit系 / Guard / Die | `EnemyBase::OnDamage`(ガードするかの判断は、ガードの作り直しのときに考える) |

## 全体の流れ(初期の遷移)

```
Patrol ──見つけた(NextOnPlayerFound)──→ Chase
Idle ──120F(NextAfterIdle)──→ AttackPre / Chase / Caution
Chase ──着いた(NextOnArrived)──→ AttackPre / Caution
Caution ──60F(NextAfterCaution)──→ Chase
AttackPre ──40F──→ Attack ──終わり(NextAfterAttack)──→ AttackEnd ──40F(NextAfterAttackEnd)──→ Back ──600離れた──→ Idle
被弾・ダウン・ガード系 ──立ち直った(NextAfterRecover)──→ Idle
```

## この先の作業で使う場所

- **後隙**: `EnemyAttackEnd` を作った(2026-10-12)。長さは今は `kAttackEndTime`(40F)固定。攻撃データを外部化したら攻撃ごとに持たせる。
- **攻撃権**: `NextAfterIdle` / `NextOnArrived` で、攻撃権が取れたら AttackPre、取れなければ Caution を返す。
- **攻撃パターン**: どの攻撃を出すかを `NextAfterIdle` / `NextOnArrived` で選び、AttackPre に渡す。
- **反撃する敵**: `NextAfterRecover` で、すぐ AttackPre を返す。

## 上書きの例(新しい敵を作るとき)

```cpp
//EnemyXxx.h
protected:
    std::shared_ptr<EnemyStateBase> NextAfterAttack() override;

//EnemyXxx.cpp
std::shared_ptr<EnemyStateBase> EnemyXxx::NextAfterAttack()
{
    //攻撃の後に下がらず、すぐ様子見に入る敵
    return std::make_shared<EnemyCaution>(GetWeakPtr());
}
```
