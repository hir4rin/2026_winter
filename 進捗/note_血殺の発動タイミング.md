# 【C++/DXライブラリ】必殺技「血殺」を、スキル攻撃の多段の最後の一撃で発動させる

自作の3Dアクションゲーム（C++ / DXライブラリ）で、必殺技「血殺」の演出を始めるタイミングを考えました。

やりたいことはこうです。

- 血殺の演出を始めるのは、**スキル1の多段攻撃の最後の一撃**と、**スキル2の多段攻撃の最後の一撃**
- **スキル0（打ち上げ）では始めない**
- ただし、攻撃の種類（`attackType`）は敵のガード判定でも使っているので、**ここは変えたくない**

この記事では、考えた3つの案と、どれを選ぶかをまとめます。

## 前提：スキル攻撃の段

スキル攻撃は、コンボのデータ（`ComboChain01.csv`）の1行が1段です。

| 名前 | index | 定数 | 多段か |
|---|---|---|---|
| スキル0（打ち上げ） | 15 | `ComboIndex::SkillAttack0` | 単発 |
| スキル弱1 | 16 | `ComboIndex::SkillAttack1` | 多段 |
| スキル弱2 | 17 | `ComboIndex::SkillAttack2` | 多段 |
| スキル弱3 | 18 | `ComboIndex::SkillAttack3` | 単発 |
| 空中スキル弱1 | 19 | `ComboIndex::AirSkillAttack1` | 多段 |

どの段も `attackType` は `SkillAttack`（2）です。
敵の `OnDamage` は「`SkillAttack` ならガードしない」という判定にこれを使っているので、スキル0だけ別の値にする、という変え方はできません。

## 今の血殺の発動条件

攻撃判定 `AttackCol` が敵に当たったときに、次の条件で血殺の演出を始めています。

```cpp
bool isUltimating = player->GetIsUltimating() && m_attackData->attackType == AttackType::SkillAttack;

//もしプレイヤーの必殺技攻撃だったら
if (GetTag().role == Collider::ColRole::UltAttack || isUltimating)
{
	//演出が始まっていなかったら始める(SetUltStart、スロー、カメラ)
}
```

この書き方だと、必殺技状態で当てたスキル攻撃なら**どの段のどの一撃でも**演出が始まってしまいます。スキル0も入ります。

## 使えるところ：多段ヒットの「最後の区間」

多段ヒットは `PlayerStateAttackBase::UpdateAttackWindow` が処理しています。
CSV には攻撃判定の区間（始まりと終わりのフレーム）が複数書けるようになっていて、新しい区間に入ったフレームで判定の設定をし直しています。

ここで、**もう「最後の区間かどうか」を計算しています**。

```cpp
//ノックバックは最後の一撃だけ//途中で吹っ飛ばすと残りが当たらないので
bool isLastHit = (activeWindow == static_cast<int>(node.attackColStartFrames.size()) - 1);
if (isLastHit)
{
	col.SetKnockBack(Vector3(node.knockBackXZ, node.knockBackY, 0), node.isKirimomi);
}
else
{
	col.SetKnockBack(Vector3(0, 0, 0), false);
}
```

スキル1の分身の位置に置く判定（`SkillAttackCol`）も、同じ `UpdateAttackWindow` を通っています。
つまり、**ここで判定に「最後の区間だよ」と教えれば、スキル1（地上・空中）とスキル2をまとめて扱えます**。

## 案A：「多段攻撃の最後の区間」を判定に持たせる（おすすめ）

### 1. AttackCol にフラグを足す

```cpp
void SetIsMultiLastHit(bool value) { m_isMultiLastHit = value; }

bool m_isMultiLastHit = false;//多段攻撃の最後の区間か
```

### 2. UpdateAttackWindow で入れる

`SetKnockBack` と同じところで入れます。区間が1つしかない攻撃（単発）は false にします。

```cpp
const bool isMulti = node.attackColStartFrames.size() > 1;
col.SetIsMultiLastHit(isMulti && isLastHit);
```

### 3. 血殺の条件を変える

```cpp
bool isUltimating = player->GetIsUltimating() &&
	m_attackData->attackType == AttackType::SkillAttack && m_isMultiLastHit;
```

### よいところ

- **スキル0は単発なので、自動で外れる**。スキル3も単発なので外れる
- 空中スキル弱1（空中から直接出すスキル1）でも発動する
- 弱攻撃5のような通常攻撃の多段は、`attackType` が `SkillAttack` ではないので入らない
- `attackType` には触らないので、**ガードの判定に影響しない**
- 足すのは数行だけ

### 気になるところ

「スキル攻撃の多段なら全部」という決め方なので、あとで多段のスキル攻撃を増やすと、それでも発動するようになります。

## 案B：段を名前で指定する

案Aの3に、段数のチェックを足します。

```cpp
const int comboIndex = player->GetComboInfo().currentComboIndex;
const bool isUltTriggerSkill = comboIndex == ComboIndex::SkillAttack1 ||
	comboIndex == ComboIndex::AirSkillAttack1 ||
	comboIndex == ComboIndex::SkillAttack2;
```

### よいところ

「スキル1とスキル2だけ」とはっきりする。多段のスキル攻撃を増やしても勝手には入らない。

### 気になるところ

発動する段を変えたいときは、コードを直す必要がある。

## 案C：CSV に「血殺を始める段」の列を足す

`ComboChain01.csv` に `ultTrigger` のような列（1 なら血殺を始める）を足して、`ComboNode` に読み込みます。
`UpdateAttackWindow` で、最後の区間のときだけ判定に入れます。

```cpp
col.SetIsUltTrigger(node.ultTrigger && isLastHit);
```

### よいところ

どの段で始めるかを CSV で決められるので、調整は一番楽。

### 気になるところ

列を足すと、後ろの列番号（`ComboNodeType`）を1つずつずらす作業が要る。手間は一番多い。

## 比べるとこうなる

| | 足すもの | スキル0を外す方法 | 段の変えやすさ |
|---|---|---|---|
| 案A | フラグ1つ＋数行 | 単発なので自動で外れる | 多段なら全部入る |
| 案B | 案A＋段数チェック1行 | 段数で指定 | コードを直す |
| 案C | CSV の列＋読み込み | CSV で指定 | CSV だけで変えられる |

## どれにするか

**まず案Aにします。** 少ない変更でスキル0が自然に外れ、ガードの判定にも影響しません。
発動する段を厳しく決めたくなったら、案Bの段数チェックを1行足せば済みます。

## 注意：if の中にある別の処理

今は、必殺技状態で当たったスキル攻撃が全部 `if` の中に入っています。
この中には、演出を始める処理（`SetUltStart`）のほかに、被ダメのエフェクトをカメラに向ける処理もあります。

最後の一撃だけが入るようにすると、途中の一撃ではエフェクトの向きをそろえなくなります。
今はエフェクトの再生がコメントアウトされているので影響はないはずですが、エフェクトを戻すときは気をつけます。

## まとめ

- 血殺を始めるのは「スキル1・スキル2の多段の最後の一撃」
- `attackType` はガードの判定で使っているので変えない
- 多段ヒットの処理（`UpdateAttackWindow`）がもう「最後の区間」を知っているので、それを判定に持たせる
- 単発のスキル0・スキル3は自動で外れる
