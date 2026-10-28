# 【C++/DXライブラリ】ボスの「ミサイルの雨」をどう作るか

自作の3Dアクションゲーム（C++ / DXライブラリ）で、ボス（Kingmech）に「背中のパックからミサイルを撃ち、プレイヤーの周りに降らせる」攻撃を入れます。
この記事では、作る前に決めた設計（当たり判定の取り方・クラスの分け方・段階の進め方）をまとめます。

---

## 1. 当たり判定の取り方：弾を飛ばすか、着弾で判定を出すか

考えた方法は2つです。

| | A. 弾を本当に飛ばす | B. エフェクトで見せて、着弾で判定 |
|---|---|---|
| 見た目 | 弾のモデル・エフェクトを動かす | ロケットはエフェクトだけ |
| 当たり判定 | 弾そのものに判定。毎フレーム動かす | 着弾した瞬間だけ、地面に判定を出す |
| 予告の円との相性 | 弾の動きとずれやすい | 場所と時間を先に決めるので、ぴったり合う |
| プレイヤーの感じ方 | 空中の弾にかすって当たると理不尽 | 「円の中にいたから当たった」と分かる |
| 作る量 | 移動・地形との当たり・弾との当たり | 判定を数フレーム出すだけ |

**B にしました。**
ミサイルの雨は「落ちてくる場所から逃げる」技なので、「着弾したときにそこにいたか」で当たりを決めるほうが分かりやすいからです。

A が向いているのは、ボスが**横から狙って撃つ**ミサイルで、プレイヤーが斬って落としたり弾き返したりする遊びを入れたい場合です。

---

## 2. 全体の形

```
KingmechMissileRain（攻撃ステート）
    アニメを流して、決めたフレームで「撃つ」だけ
        │ owner->FireMissile(着弾位置)
        ▼
BossEnemyKingmech（ボス本体）
    飛んでいるミサイルの一覧を持ち、毎フレーム Update する
        │
        ▼
KingmechMissile（ミサイル1発分）
    段階(enum)・タイマー・エフェクト・攻撃判定を持つ
```

### ミサイルをステートに持たせない理由
攻撃ステートは、撃ち終わると AttackEnd（後隙）→ Idle へ移って消えます。
でも、そのときにはまだ空中にあるミサイルが残っています。ステートに持たせると、ステートと一緒に消えてしまいます。

なので、**ミサイルの一覧はボス本体が持ちます**。ステートは「撃つ」と頼むだけです。

---

## 3. ミサイル1発のクラス

着弾する場所を決めた時点で1発ずつ作り、ミサイル自身が **enum の段階**を持って進みます。

```cpp
class KingmechMissile
{
public:
	enum class Phase
	{
		Warning,//予告の円だけ出ている
		Falling,//上からロケットが落ちてくる(見た目だけ)
		Explode,//着弾//判定を数フレームだけ出す
		Finished,//終わり//一覧から消してもらう
	};
	KingmechMissile(std::weak_ptr<CharacterBase> owner, const Vector3& landPos);
	void Update();
	bool IsFinished()const { return m_phase == Phase::Finished; }
private:
	void ChangePhase(Phase next);
	Phase m_phase = Phase::Warning;
	float m_timer = 0.0f;
	Vector3 m_landPos;//着弾する位置(地面の高さ)
	int m_warningEffect = -1;//予告の円
	std::shared_ptr<AttackCol> m_attackCol;//爆発の判定
};
```

### 段階ごとの中身（フレームは目安）

| 段階 | 長さ | やること |
|---|---|---|
| **Warning** | 45F | 着弾位置に予告の円。だんだん赤くする・縮めるなどで「もうすぐ落ちる」を見せる |
| **Falling** | 15F | 上空から落ちてくるロケットのエフェクト。15F でちょうど地面に届くように作る |
| **Explode** | 4F | 爆発のエフェクト・カメラシェイク。攻撃判定を有効にする |
| **Finished** | - | 判定を消す。ボスが一覧から外す |

```
撃つ ─── Warning(45F) ─── Falling(15F) ─── Explode(4F) ─── Finished
         予告の円              ロケットが落ちる     爆発・判定ON       消える
```

- **Warning と Falling を分ける理由**：「予告は長く、落ちる瞬間は速い」形にしやすく、見てからよけられる時間を調整しやすいからです。
- **タイマーはタイムスケールを掛けて進める**（`GetTimeScale() * ownTimeScale`）。エフェクト（EffectManager）もタイムスケールに従うので、血殺などのスローのときにエフェクトと判定がずれません。

---

## 4. 判定を着弾地点に置く（今のしくみでは足りない所）

今の `AttackCol::ApplyPos` は、毎フレーム**持ち主（ボス）の位置＋前方向**に判定を置き直します。

```cpp
void AttackCol::ApplyPos()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	m_rb.m_pos = owner->GetRigidBody().GetPos();
	m_rb.m_pos += owner->GetTargetVec() * kAttackColOffset;
}
```

着弾地点に置いても、次のフレームでボスの所へ戻ってしまいます。
そこで、**`ApplyPos` を上書きして位置を動かさない子クラス**を作ります。

```cpp
//決めた位置から動かない攻撃判定(ミサイルの着弾など)
class FixedAttackCol : public AttackCol
{
public:
	FixedAttackCol(std::weak_ptr<CharacterBase> owner, const CharacterBase::AttackData& data, const Vector3& pos);
	void ApplyPos()override { m_rb.m_pos = m_fixedPos; }
private:
	Vector3 m_fixedPos;
};
```

### 判定の形
- 今の攻撃と同じ**球**（`SphereShape`）で、**中心を地面の高さ**にします。下半分は地面に埋まるので、実質**半球**になります。
- ジャンプでよけられるかどうかは、球の半径で決まります。
- `ColInit` の `lifeTime` を使えば、決めた時間で自動で消えます（プレイヤーの着地の衝撃 `m_burstAttackCol` と同じやり方）。

### 気にしなくていいこと・あとで決めること
- **爆発が重なったときの2回ヒット**：当たったかどうかの記録（`m_hitIds`）は判定ごとなので、2発の爆発が重なると2回当たる可能性があります。ただ、プレイヤーには被弾後の無敵時間があるので、実際は1回分しか入りません。
- **ジャスト回避**：敵の判定なので、ジャスト回避の判定にも反応します。ミサイルをジャスト回避できるようにするかは、あとで決めます。

---

## 5. 着弾する場所の決め方

撃つときに、次の順で決めます。

1. **1発目はプレイヤーの今の位置**（止まっていると当たる）
2. 残りは**プレイヤーの周りにランダム**（半径 300〜800 など）。前のミサイルと近すぎたら引き直して、逃げ道を作る
3. 高さは、その位置の上から下へ線分の判定（`MV1CollCheck_Line`）を当てて、**床の高さ**にする。床が無い場所（崖の外）なら、その1発は撃たないか、別の場所で引き直す

---

## 6. ボス本体でやること

```cpp
std::vector<std::unique_ptr<KingmechMissile>> m_missiles;//飛んでいるミサイル

void FireMissile(const Vector3& landPos);//1発追加する
```

- `Update` で全部のミサイルの `Update` を呼び、`IsFinished()` のものを消す
- ボスが死んだ・消えたときに、残りのミサイルを落とし切るか消すかを決める（**消すほうが簡単**）

---

## 7. 攻撃ステート（KingmechMissileRain）

攻撃CSVで普通の攻撃と同じ1行として登録し、特殊攻撃として専用のステートにします。

1. 攻撃CSVの最後に `special` 列を足し、ミサイルの雨の行に `MissileRain` と書く
2. `EnemyAttackPre` で `EnemyAttack` を直接作っている所を `owner->CreateAttackState()`（仮想関数）にする
3. Kingmech の `CreateAttackState()` で、`special` が `MissileRain` なら `KingmechMissileRain` を返す
4. `KingmechMissileRain` は **`EnemyAttack` を継承**する。攻撃の流れ・攻撃権・攻撃中は怯まない処理がそのまま使える

ステートの中身は2つだけです。

- 攻撃CSVのアニメ（Roar）を流す
- 撃ち始めのフレーム（`attackColStart` を流用）から、`kFireInterval` ごとに `kMissileNum` 発、`FireMissile` を呼ぶ

撃ち終わったら、普通の攻撃と同じく AttackEnd（後隙）へ移ります。
**ミサイルが降っている間はボスの後隙**になるので、「よけながら近づいて殴る」流れが作れます。

---

## 8. 調整する値

| 定数 | 意味 | 目安 |
|---|---|---|
| `kMissileNum` | 1回で撃つ数 | 6〜10 |
| `kFireInterval` | 撃つ間隔 | 5〜8F |
| `kWarningTime` | 予告の円の長さ | 45F |
| `kFallTime` | 落ちてくる長さ | 15F |
| `kExplodeTime` | 判定を出す長さ | 4F |
| `kExplodeRadius` | 爆発の半径 | 150〜250 |
| `kScatterMin` / `kScatterMax` | プレイヤーからの散らばり | 300 / 800 |
| `kMinSpacing` | ミサイル同士の最短距離 | 200 |

最初はステートやミサイルの .cpp の先頭の定数にしておき、調整が多くなったら CSV に出します。

---

## 9. 作る順番

1. `FixedAttackCol`（位置を動かさない判定）
2. `KingmechMissile`（段階・タイマー・判定。エフェクトは仮でデバッグ描画の円）
3. ボスの一覧と `FireMissile`、F キーなどで1発撃って確認
4. `special` 列・`CreateAttackState()`・`KingmechMissileRain`
5. Effekseer で予告の円・ロケット・爆発のエフェクトを作って差し替え
