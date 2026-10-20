# 【C++/DXライブラリ】血殺中にプレイヤーを分身のポーズで止める：スキル攻撃をアニメではなくフレームで進める

自作の3Dアクションゲーム（C++ / DXライブラリ）で、スキル攻撃中の見せ方を変えようとしています。

やりたいことはこうです。

- 血殺の演出中は、分身エフェクトを止めて**プレイヤー本体を表示する**
- そのとき、プレイヤーは**分身と同じポーズで止まっている**ようにする
- ついでに、スキルを連打したとき血殺の演出中に次のスキルへ移ってしまう不具合も直す

この記事では、何を変える必要があるのかと、その手順をまとめます。

## 今の仕組み

スキル1・2では、次のようにしています。

- プレイヤーのモデルを描画しない（`m_isSkillInvisible = true`）
- 代わりに分身エフェクト（`GhostDash` / `GhostDash3D`）を出す
- プレイヤーには、CSVに書いた**長いフレームのアニメ**を流している（見えないので、時間を数えるためだけのアニメ）

分身エフェクトが使っているモデルは `Ghost_Dodge_F_13.efkmodel` です。
名前から、**回避アニメの13フレーム目**のポーズだと思われます。

| エフェクト | モデル | 元のポーズ（名前からの推測） |
|---|---|---|
| `GhostDash`（スキル1） | `Ghost_Dodge_F_13` | 回避の13フレーム目 |
| `GhostDash3D`（スキル2） | `Ghost_Dodge_F_13` | 同じ |
| `GhostSkill3_Fall`（スキル3の落下） | `Ghost_AirSlam_Fall` | AirSlam の落下中 |
| `GhostSkill3_Impact`（スキル3の着地） | `Ghost_AirSlam_Impact` | AirSlam の着地 |

アニメCSV（`Player01Anim.csv`）には `Dodge_F` という名前が無いので、近い名前の `Dodge_Combat_F` が候補です。違ったら、前回避で使っている `Roll_Combat_F_0` を試します。

## 問題：スキル攻撃の進み方がアニメのフレームに頼っている

ポーズを止めるには、アニメの再生速度を0にして13フレーム目で止めればよさそうです。

```cpp
player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, "Dodge_Combat_F", false, 0.0f, -1.0f, 13.0f, 0.0f);
//再生速度0・終わり指定なし・開始13F・ブレンド0
```

ところが、スキル攻撃は**「今アニメの何フレーム目か」で全部の判断をしている**ので、アニメを止めると攻撃も止まってしまいます。
次の章で、アニメのフレームを見ている場所を1つずつ見ていきます。

## アニメのフレームを見ている場所

行番号は 2026-10-20 時点のものです。

| 場所 | 見ているもの |
|---|---|
| `PlayerStateSkillAttack.cpp:97-99` | `cancelFrame` / `actionCancelFrame` |
| `PlayerStateSkillAttack.cpp:192` | `GetAnimEndFlag`（スキルの終わり） |
| `PlayerStateSkillAttack.cpp:270` | コンボ入力の受付区間 |
| `PlayerStateAttackBase.cpp:265` / `:277` | 突進の区間（`moveStartFrame` / `moveEndFrame`） |
| `PlayerStateAttackBase.cpp:340`（`UpdateAttackWindow`） | 多段ヒットの区間と `isLastHit`。**血殺の発動条件そのもの** |
| `PlayerStateAttackBase.cpp:382`（`UpdateDropAttackWindow`） | スキル3の落下中の追加ヒット |
| `ResolveTransitionFrame` | CSVが空欄のときの割合（0.5など）を、アニメの総フレーム数から出している |
| `AttackCol.cpp:145` | `knockBackFrame -= nowAnimFrame`。State の外からプレイヤーのアニメのフレームを読んでいる |

### 1. cancelFrame / actionCancelFrame（`PlayerStateSkillAttack.cpp:97-99`）

```cpp
float nowFrame = player->m_anim.GetNowAnimFrame();
float actionCancelFrame = ResolveTransitionFrame(currentNode.actionCancelFrame, kDefaultActionCancelRate);
float cancelFrame = ResolveTransitionFrame(currentNode.cancelFrame, kDefaultCancelRate);
```

`nowFrame` は、`Update` の中で次のことに使われています。

- **スキル0（打ち上げ）**：`cancelFrame` を過ぎたら、当たっていればスキル1へ、外れていれば Idle/Move へ（106〜125行）
- **回避・ジャンプ・確殺のキャンセル**：`actionCancelFrame` を過ぎたら受け付ける（151行）
- **予約した次の攻撃へ移る**：`cancelFrame` を過ぎたら（175行）

**アニメを止めると**：`nowFrame` が13のまま進まないので、いつまでもキャンセルできず、次の段にも移れません。

### 2. スキルの終わり（`PlayerStateSkillAttack.cpp:192`）

```cpp
//コンボ予約がなく、アニメーションが終わったとき
if (player->m_anim.GetAnimEndFlag())
```

`GetAnimEndFlag` は、`Animation::Update` の中で「今のフレーム ≧ 終わりのフレーム」になったときに立ちます（`Animation.cpp:82-84`）。

**アニメを止めると**：再生速度0ではフラグが立たないので、**スキル攻撃から抜けられなくなります**。

### 3. コンボ入力の受付区間（`PlayerStateSkillAttack.cpp:268-271`）

```cpp
float inputStartFrame = ResolveTransitionFrame(currentNode.comboInputStartFrame, kDefaultComboInputStartRate);
float inputEndFrame = ResolveTransitionFrame(currentNode.comboInputEndFrame, kDefaultComboInputEndRate);
float nowFrame = player->m_anim.GetNowAnimFrame();
bool canInput = nowFrame >= inputStartFrame && nowFrame <= inputEndFrame;
```

LB（次のスキル）・X（弱攻撃）・Y（強攻撃）の予約を、この区間でだけ受け付けています。

**アニメを止めると**：13フレームが区間の中なら受付がずっと開いたまま、外なら一度も開きません。

### 4. 突進の区間（`PlayerStateAttackBase.cpp:265` / `:277`）

`AttackMoveMent` の中です。上下差の無い攻撃と、ある攻撃で分かれています。

```cpp
//上下差なし：この区間だけ前に突進する
if (player->m_anim.IsAnimFrameBetween(node.moveStartFrame, moveEndFrame))

//上下差あり：moveStartFrameまでは動かない(重力も足さない)
if (player->m_anim.GetNowAnimFrame() < node.moveStartFrame)
```

**アニメを止めると**：13が区間の中ならずっと突進し続け、外なら一度も動きません。上下差のある攻撃（スキル3など）は、`moveStartFrame` が13より大きいと動き出しません。

### 5. 多段ヒットの区間と isLastHit（`PlayerStateAttackBase.cpp:340`、`UpdateAttackWindow`）

```cpp
for (int i = 0; i < static_cast<int>(node.attackColStartFrames.size()); ++i)
{
	if (player->m_anim.IsAnimFrameBetween(node.attackColStartFrames[i], node.attackColEndFrames[i]))
	{
		activeWindow = i;
		break;
	}
}
//新しい区間に入った1フレームだけ、ヒット履歴を消して、最後の区間ならノックバックと isLastHit を入れる
bool isLastHit = (activeWindow == static_cast<int>(node.attackColStartFrames.size()) - 1);
col.SetIsLastHit(isLastHit);
```

CSVには攻撃判定の区間（始まりと終わりのフレーム）を複数書けます。今どの区間にいるかを、アニメのフレームで探しています。

血殺は、`AttackCol` で「`isLastHit` が true のスキル1・2の攻撃が当たったとき」に始まります（`AttackCol.cpp:174-193`）。
つまり**ここは血殺の発動条件そのもの**です。

**アニメを止めると**：区間が変わらないので、多段ヒットにならず、最後の区間にも入りません。**血殺が発動しなくなります。**

地上スキル1の分身の位置に出す判定（`SkillAttackCol`）も、同じ `UpdateAttackWindow` を通っています（`PlayerStateSkillAttack.cpp:574`）。

### 6. スキル3の落下中の追加ヒット（`PlayerStateAttackBase.cpp:382`、`UpdateDropAttackWindow`）

5と同じ区間の探し方で、落下中に何回かダメージを入れるための判定（`m_dropAttackCol`）をON/OFFしています。

スキル3は分身の姿勢（AirSlam）が回避の13フレーム目とは違うので、**ポーズを止めるのがスキル1・2だけなら、ここは影響しません**。ただ、Base の関数なので、5を直すなら同じように直しておくのが自然です。

### 7. ResolveTransitionFrame（`PlayerStateAttackBase.cpp:523-531`）

```cpp
//CSVで指定されていたらそのフレームを使う
if (csvFrame >= 0.0f) return csvFrame;
//負の値なら、デフォルトの進行率をフレームに変換する
return player->m_anim.GetAnimEndFrame() * defaultRate;
```

CSVが空欄（-1）のときは、「アニメの長さ × 割合（0.5 など）」をフレームにしています。

**アニメを差し替えると**：割合をかける元が回避アニメの長さになるので、空欄のフレームが全部ずれます。
使っているのは、1と3のフレーム（キャンセル・入力受付）と、4の `moveEndFrame` です。

### 8. AttackCol の knockBackFrame（`AttackCol.cpp:145-146`）

```cpp
float nowAnimFrame = player->GetAnimation().GetNowAnimFrame();
m_attackData->knockBackFrame -= nowAnimFrame;
```

`knockBackFrame` は、最初は「攻撃のアニメの総フレーム数」です（`CreateAttackCol`）。
当たった瞬間にアニメの今のフレームを引くので、**「攻撃が終わるまでの残りフレーム」**になります。
これが敵の吹き飛び時間（`HitInfo::duration` → `EnemyHitGround` など）になり、プレイヤーの攻撃が終わるまで敵がひるんでいるようにしています。

これだけ **State の外**（当たり判定の処理の中）から、プレイヤーのアニメを直接読んでいます。

**アニメを止めると**：常に13が引かれるので、吹き飛び時間が「総フレーム − 13」になり、実際の残り時間と合わなくなります。

## 変更の手順

### 手順1：スキル攻撃を自前のフレームで進める

**① 変数を持たせる**（`PlayerStateSkillAttack.h`）

```cpp
float m_frame = 0.0f;//経過フレーム
float m_endFrame = 0.0f;//終わりのフレーム
float m_frameSpeed = 1.0f;//CSVの再生速度
```

**② Enter で初期化する**（アニメを変える54行の前）

```cpp
m_frame = 0.0f;
m_frameSpeed = node.animTimeScale;
float total = player->m_anim.GetAnimTotalFrame(node.animName);
m_endFrame = (node.endFrame < 0.0f) ? total : (std::min)(node.endFrame, total);
```

終わりのフレームは `Animation::ResolveEndFrame` と同じ決め方です。これで、**CSVのフレームの値はそのまま使えます**。

**③ フレームを進める**

`Update` の中の `player->m_anim.Update();`（129・142・217行）の横で進めます。

```cpp
m_frame += System::GetInstance().GetTimeScale() * m_frameSpeed;
```

`Animation::Update` と同じ式なので、スローのかかり方も今までと同じです。

**④ アニメのフレームを見ている場所を置き換える**

- スキル攻撃の中（上の表の1〜3）は、`m_frame` / `m_endFrame` に直接置き換えます。
- Base の中（4〜7）は通常攻撃と共通なので、**仮想関数にする**のがおすすめです。

```cpp
//PlayerState.h
virtual float GetAttackFrame();//基本はアニメのフレームを返す
virtual float GetAttackEndFrame();//基本はアニメの終わりのフレームを返す

//PlayerStateSkillAttack.h
float GetAttackFrame()override { return m_frame; }
float GetAttackEndFrame()override { return m_endFrame; }
```

Base の `player->m_anim.IsAnimFrameBetween(...)` / `GetNowAnimFrame()` / `GetAnimEndFrame()` を、これらに置き換えます。
`IsAnimFrameBetween` の代わりは、Base に小さい関数を作ると楽です（`frame >= start && frame < end`。`Animation` と同じ境界にする）。

- 8の `AttackCol` は State の外なので、`Player` に「今のStateの `GetAttackFrame()` を返す関数」を作り、それを使います。

通常攻撃などほかのステートは、基本の実装（アニメのフレーム）を返すので、動きは変わりません。

### 手順2：スキル1・2の姿勢を13フレーム目で止める

Enter の `ChangeAnimWithModelHandle`（54行）を、スキル1・2のときだけ差し替えます。

```cpp
if (IsSkillAttack1(currentComboIndex) || currentComboIndex == ComboIndex::SkillAttack2)
{
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, kGhostPoseAnim, false, 0.0f, -1.0f, kGhostPoseFrame, 0.0f);
}
```

アニメ名とフレームは、無名namespaceに定数で置いておくと直しやすいです。
`m_anim.Update()` はそのまま呼んでかまいません（再生速度0なので姿勢は動きません）。

### 手順3：血殺のときに分身を消してプレイヤーを出す

`Update` の中で、次の2つがそろったら `StopGhostEffect()` を呼びます。

- `System::GetInstance().GetBattleMgr()->GetIsUltimating()` が true
- `m_ghostEffectHandle != -1`

`StopGhostEffect` は `m_isSkillInvisible = false` もするので、これだけでプレイヤーが表示に戻ります。
あとで消すかもしれないので、`constexpr bool kShowPlayerOnKessatsu` のようなスイッチで囲っておくと、false にするだけでやめられます。

**注意**：地上スキル1の分身は**敵の位置**に出しています（`GetSkillEffectBasePos`）。表示したプレイヤーは、分身がいた場所ではなくプレイヤー自身の位置に出るので、見た目がおかしければ位置をそろえる処理が要ります。

## 連打したときの不具合

### 何が起きているか

血殺の演出中（`BattleManager::GetIsUltimating()` の間）でも、スキル攻撃のステートは普段通りに動いています。

1. スキル1の最後の一撃で血殺が始まる（`AttackCol.cpp:193`）。時間が0.1倍になり、120フレームの演出に入る
2. `AttackInputCheck` は演出中も LB を受け付ける。最後の一撃より前に予約していた分も残っている
3. `cancelFrame` に来ると、演出中に `StartNextSkill` でスキル2へ移る。分身がもう一度出て、突進もする
4. 回避・ジャンプ・確殺のキャンセル（151行）も、演出中に効いてしまう
5. 演出の途中でスキルが最後まで進むと、Idle などに戻る

### 直し方

`Update` の `AttackInputCheck()` の前（146行あたり）で、演出中は先に進まないようにします。

```cpp
//血殺中は入力も遷移もしない
if (System::GetInstance().GetBattleMgr()->GetIsUltimating())
{
	m_isComboInputReserved = false;//演出前の予約も捨てる
	player->m_anim.Update();//(手順1を入れたらフレームも進める)
	return;
}
```

- この位置なら、スキル0（102〜131行）とスキル3の落下（134〜144行）の処理の後なので、そちらには影響しません。
- 192行の「終わったら Idle/Move/Fall に戻る」より前で return するので、**演出が終わるまでスキル攻撃のステートに残ります**。手順2の「ポーズで止める」とも合います。
- 予約を捨てるかどうかは好みです。「演出の後に次のスキルへつなげたい」なら予約は残して、遷移だけ止めます。

### 確かめること

- 演出が終わったとき、`m_frame` がもう `m_endFrame` を過ぎていれば、次のフレームですぐステートを抜けます。余韻がほしければ、演出が終わってから数フレーム待つ処理を足します。
- 演出の後にもう一度LBを押すと、次の段の最後の一撃でまた血殺が始まります（`Player::m_isUltimating` が true のまま残るため）。これが意図通りかどうか。

## ついでに気づいたこと：knockBackFrame が当たるたびに減る

8の `knockBackFrame -= nowAnimFrame` は、`AttackCol` が持っている攻撃データ（`m_attackData`）を**直接書き換えています**。
同じ判定で2回目に当たったとき（多段ヒットの2ヒット目や、2体目の敵）は、すでに減った値からさらに引くので、吹き飛び時間がどんどん短くなり、マイナスになることもありそうです。

ゲームではまだ確かめていません。直すなら、引いた値を `m_attackData` に書き戻さず、`OnDamageInterFace` に渡す前にコピーを作ってそちらで引く形にします。

## まとめ

- ポーズを止めるとアニメのフレームが進まなくなるので、スキル攻撃は**自前のフレーム**で進める
- アニメのフレームを見ている場所は8つ。Base と `AttackCol` は通常攻撃と共通なので、**仮想関数 `GetAttackFrame()`** で切り替える
- 姿勢は `Dodge_Combat_F`（推測）の13フレーム目で止める
- 血殺に入ったら `StopGhostEffect()` で分身を消し、プレイヤーを出す
- 血殺の演出中は、入力・キャンセル・次の段への遷移を止める
