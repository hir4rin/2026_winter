# 【C++/DXライブラリ】スキル血殺の見せ方を変える：自前フレームと「旗」で実装する手順

スキル1・2の血殺中に、分身エフェクトではなくプレイヤー本体を、分身と同じ姿勢で見せるための実装手順です。

一度試しに実装して動きを確かめ、**今はすべて消してあります**（2026-11-09時点）。自分で書き直すためのメモです。
詳しい背景は `note_スキル攻撃をフレームで進める.md` にあります。

## 完成形の流れ

1. スキルの最後の一撃が当たる。血殺はまだ始めず、旗だけ立てる
2. 分身エフェクトはそのまま最後まで出る
3. スキルが終わったら血殺を始める。分身を消して、プレイヤーを回避13Fの姿勢で表示する
4. 血殺が終わったら、Idle / Move / Fall に戻る

## 決めたこと

| 項目 | 決定 | 理由 |
|---|---|---|
| 血殺の発動 | 当たったら旗を立て、スキルの終わりに発動 | 通常時のタイミングが変わらない。分身がスローでカクつかない |
| スキルの時間 | `m_frame` で自分で数える | アニメを止めてもスキルが進む |
| 姿勢 | `Dodge_Combat_F` の13Fで止める | 試した結果、分身と同じ形だった（`Roll_Combat_F_0` は頭が下で別の姿勢） |

試したときの動き（スキル2、スキル開始からの実フレーム）:

| 実フレーム | 起きたこと |
|---|---|
| 40F頃まで | `m_frame` が進み、最後の判定（30F〜）が当たって旗が立つ |
| 53F頃 | `m_frame` が41に届いて血殺が始まる |
| 54〜110F頃 | 姿勢のまま止まる |
| 115F頃 | 血殺が終わり、次の状態へ移る |

## 進め方

一度に全部入れず、段階ごとにゲームで確かめます。
**行番号は実装前のものです。** 上から足していくとずれるので、書いてある文で検索してください。

---

## 段階1：時間を自分で数える（見た目は何も変わらない）

### 1. `Character/Player/PlayerState.h`

`IsInvincible` の下に足します。

```cpp
virtual bool IsInvincible()const { return false; }
//攻撃の今のフレーム//普段はアニメのフレーム
virtual float GetAttackFrame(float animFrame)const { return animFrame; }
virtual float GetAttackEndFrame(float animEnd)const { return animEnd; }
```

### 2. `PlayerStateAttackBase`

`.h` の `ResolveTransitionFrame` の下に足します。

```cpp
float GetNowFrame();//今の攻撃のフレーム
bool IsFrameBetween(float start, float end);
```

`.cpp` の一番下に足します。

```cpp
float PlayerStateAttackBase::GetNowFrame()
{
	auto player = m_owner.lock();
	return player ? GetAttackFrame(player->m_anim.GetNowAnimFrame()) : 0.0f;
}

bool PlayerStateAttackBase::IsFrameBetween(float start, float end)
{
	const float frame = GetNowFrame();
	return frame >= start && frame < end;
}
```

`.cpp` の5か所を置き換えます。

```cpp
//266行
if (IsFrameBetween(node.moveStartFrame, moveEndFrame))

//278行
if (GetNowFrame() < node.moveStartFrame)

//341行と383行(同じ文)
if (IsFrameBetween(node.attackColStartFrames[i], node.attackColEndFrames[i]))

//533行
return GetAttackEndFrame(player->m_anim.GetAnimEndFrame()) * defaultRate;
```

424行と459行は、CSVのアニメ名で総フレームを取っているだけなので変えません。

### 3. `Player` と `AttackCol`

`Player.h` の `GetIsUltimating` の下に足します。

```cpp
float GetAttackFrame();//今の攻撃のフレーム
```

`Player.cpp` の `void Player::ChangeState(...)` の上に足します。

```cpp
float Player::GetAttackFrame()
{
	const float animFrame = m_anim.GetNowAnimFrame();
	return m_currentState ? m_currentState->GetAttackFrame(animFrame) : animFrame;
}
```

`Collision/AttackCol.cpp` の161行を置き換えます。

```cpp
float nowAnimFrame = player->GetAttackFrame();
```

**ここで一度ビルドします。** まだ `m_frame` が無いので、動きは今までと全く同じです。

### 4. `PlayerStateSkillAttack.h`

`public:` の `DebugDraw` の下に足します。

```cpp
float GetAttackFrame(float)const override { return m_frame; }
float GetAttackEndFrame(float)const override { return m_endFrame; }
```

`private:` のすぐ下に足します。

```cpp
void AdvanceFrame();//自前のフレームとアニメを進める
```

一番下の `m_isComboInputReserved` の下に足します。

```cpp
float m_frame = 0.0f;//経過フレーム
float m_endFrame = 0.0f;//終わりのフレーム
float m_frameSpeed = 1.0f;//CSVの再生速度
```

### 5. `PlayerStateSkillAttack.cpp` の `Enter`

includeの最後に足します。

```cpp
#include <algorithm>
```

56行の `ChangeAnimWithModelHandle` の**直後**に足します（56行は変えません）。

```cpp
m_frame = 0.0f;
m_frameSpeed = node.animTimeScale;
const float total = player->m_anim.GetAnimTotalFrame(node.animName);
m_endFrame = (node.endFrame < 0.0f) ? total : (std::min)(node.endFrame, total);
```

### 6. `AdvanceFrame`

`Update` 関数の閉じカッコの下に足します。

```cpp
void PlayerStateSkillAttack::AdvanceFrame()
{
	auto player = m_owner.lock();
	if (!player) return;
	m_frame = (std::min)(m_frame + System::GetInstance().GetTimeScale() * m_frameSpeed, m_endFrame);
	player->m_anim.Update();
}
```

`Update` の中の `player->m_anim.Update();`（131、144、151、226行）を置き換えます。

```cpp
AdvanceFrame();
```

**`AdvanceFrame` の中の `player->m_anim.Update();` は変えません。** 変えると自分を呼び続けて止まります。

### 7. 時計を `m_frame` に変える

```cpp
//99行と281行(同じ文)
float nowFrame = m_frame;

//201行
if (m_frame >= m_endFrame)
```

**確認:** スキル0〜3が今までと同じタイミングで動くこと。多段ヒットと次の段へのつながりも見ます。ここが一番大事です。

---

## 段階2：回避13Fで止める

`PlayerStateSkillAttack.cpp` の無名namespaceに足します。

```cpp
const char* const kKessatsuPoseAnim = "Dodge_Combat_F";//分身の姿勢のアニメ
constexpr float kKessatsuPoseFrame = 13.0f;//分身の姿勢のフレーム
```

`Enter` の `ChangeAnimWithModelHandle`（56行）を、次のように置き換えます。`m_frame` などを入れる4行の**後ろ**に置きます。

```cpp
//スキル1,2は分身の姿勢で止めておく
if (IsSkillAttack1(currentComboIndex) || currentComboIndex == ComboIndex::SkillAttack2)
{
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, kKessatsuPoseAnim, false, 0.0f, -1.0f, kKessatsuPoseFrame, 0.0f);
}
else
{
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame, 0.0f, node.blendFrame);
}
```

**確認:** アニメが止まっていても、スキルが今まで通り進んで終わること。この段階では見た目は変わりません。

---

## 段階3：旗を立てて、終わりに血殺する

### 1. `Player.h`

`GetAttackFrame` の下に足します。

```cpp
void ReserveKessatsu(const Vector3& pos) { m_isKessatsuReserved = true; m_kessatsuPos = pos; }
void ClearKessatsuReserve() { m_isKessatsuReserved = false; }
bool IsKessatsuReserved()const { return m_isKessatsuReserved; }
const Vector3& GetKessatsuPos()const { return m_kessatsuPos; }
void StartSkillKessatsu(const Vector3& pos);
```

`m_isSkillInvisible` の下に足します。

```cpp
bool m_isKessatsuReserved = false;//スキルの終わりに血殺する
Vector3 m_kessatsuPos = {};//血殺の位置
```

### 2. `Player.cpp`

includeに足します。

```cpp
#include "Battle/BattleManager.h"
```

無名namespaceに足します（`AttackCol.cpp` の32〜34行から移します。`AttackCol.cpp` 側は消します）。

```cpp
constexpr int kSkillKessatuFrame = 60;//スキル血殺の演出の長さ(タイムスケールを掛けて減る)
constexpr int kSkillKessatuSlowFrame = 60;//スキル血殺のスローの長さ(実フレーム)
constexpr float kSkillKessatuTimeScaleRate = 0.1f;//スキル血殺中の時間スケール
```

`GetAttackFrame` の下に足します。

```cpp
void Player::StartSkillKessatsu(const Vector3& pos)
{
	auto battleMgr = System::GetInstance().GetBattleMgr();
	battleMgr->SetUltStart(kSkillKessatuFrame, pos);
	if (battleMgr->GetIsLastHitEventPlaying())return;
	System::GetInstance().SetTimeScaleForFrames(kSkillKessatuTimeScaleRate, kSkillKessatuSlowFrame);
	if (auto cameraManager = m_cameraManager.lock())
	{
		cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::SkillKessatuCamera);
	}
}
```

### 3. `AttackCol.cpp`

`//スキルと必殺の血殺を分ける` の下の `if (isUltimating)` の中身を置き換えます。

```cpp
if (isUltimating)
{
	//スキル1・2は終わりに発動//スキル3の着地はすぐ
	if (m_attackData->attackType != AttackType::ImpactSkillAttack)
	{
		player->ReserveKessatsu(other.GetWorldPos());
	}
	else
	{
		player->StartSkillKessatsu(other.GetWorldPos());
	}
}
```

### 4. `PlayerStateSkillAttack.h`

`AdvanceFrame` の下に足します。

```cpp
void EndSkill();//Idle/Move/Fallへ戻す
void StartKessatsu();//分身を消して血殺に入る
```

メンバーに足します。

```cpp
bool m_isKessatsuPose = false;//血殺の姿勢で止まっている
```

### 5. `PlayerStateSkillAttack.cpp`

**スキルの終わりの処理を切り出します。** `if (m_frame >= m_endFrame)` の中を置き換えます。

```cpp
if (m_frame >= m_endFrame)
{
	EndSkill();
	return;
}
```

```cpp
void PlayerStateSkillAttack::EndSkill()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->m_rb.m_vel.y = 0.0f;//y軸の速度を0に戻す
	if (!player->IsFloor())//空中は落下状態に移行
	{
		player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));
		return;
	}
	if (Input::GetInstance().IsLeftStickInput())
	{
		player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
		return;
	}
	player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
}
```

**血殺に入る関数を足します。**

```cpp
void PlayerStateSkillAttack::StartKessatsu()
{
	auto player = m_owner.lock();
	if (!player) return;
	m_isKessatsuPose = true;
	StopGhostEffect();//分身を消してモデルを出す
	player->m_rb.m_vel = Vector3(0, 0, 0);
	m_attackCol->SetIsActive(false);
	if (m_skillAttackCol)m_skillAttackCol->SetIsActive(false);
	player->StartSkillKessatsu(player->GetKessatsuPos());
}
```

**`Update` の `AttackMoveMent();` の前に足します。** 止めた13Fを見て判定が復活しないように、移動も判定も通しません。

```cpp
//血殺の姿勢の間は移動も判定も通さない
if (m_isKessatsuPose)
{
	if (System::GetInstance().GetBattleMgr()->GetIsUltimating())
	{
		player->m_anim.Update();
		return;
	}
	EndSkill();
	return;
}
```

**`//コンボ予約の入力を取る` の前に足します。** 旗が立ったら、次の段や回避へのキャンセルはここで止まります。

```cpp
//旗が立ったらキャンセルせず、終わりで血殺
if (player->IsKessatsuReserved())
{
	if (m_frame >= m_endFrame)
	{
		StartKessatsu();
		return;
	}
	AdvanceFrame();
	return;
}
```

**`Exit` の `player->m_comboInfo.isHit = false;` の下に足します。**

```cpp
player->ClearKessatsuReserve();
```

**確認:** 最後の一撃のあと、分身が出終わってから血殺が始まり、プレイヤーが回避の姿勢で表示されること。血殺が終わったら立ち状態か落下に戻ること。連打しても途中で次の段へ移らないこと。

---

## 段階4：見た目の調整（必要なら）

- **地上スキル1の位置:** 分身は敵の位置に出ているので、`StartKessatsu` で本体を敵の少し手前へ移さないと、表示した瞬間に位置が飛びます。スキル0から通して確認します。
- **白い血:** きっかけの一撃は `SetUltStart` より先に血を出すので、白い血になっていないようです（コードを読んだ範囲）。`StartKessatsu` で `BloodUltEffect` を1回出すと、血殺の始まりに合います。
- **血殺のカメラ:** スキル2では、プレイヤーが画面の左下に一部しか映りませんでした。全身を見せるなら調整が必要です。
- **体が水平のまま:** 地上でも飛び込むような水平の姿勢で止まります。分身と同じ姿勢ですが、地面の近くでは違和感があるかもしれません。
- **黒い片翼:** プレイヤーを表示すると片翼も一緒に出ます。消すなら、姿勢の間だけ条件を足します。

## 注意

- 確認は、一時コードでスキルを直接出して行いました。実際にボタンで操作しての確認はまだです。
- 姿勢の間に `Update` で呼ぶのは `AdvanceFrame` ではなく `player->m_anim.Update()` です（`m_frame` はもう終わりまで進んでいるので、どちらでも動きは同じです）。
- `knockBackFrame -= nowAnimFrame` は攻撃データを直接書き換えているので、多段ヒットや2体目でひるみ時間が減っていきそうです（コードを読んだ範囲。今回の話とは別）。
