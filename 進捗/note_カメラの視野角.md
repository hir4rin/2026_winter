# 【C++/DXライブラリ】カメラの視野角をブレンド・演出できるようにした話

自作の3Dアクションゲーム（C++ / DXライブラリ）で、カメラの視野角（FOV）を演出に使えるようにしました。
きっかけは、空中暗殺のカメラで視野角を狭めたら、演出が終わったときに画面が「カクっ」となったことです。

この記事では、視野角を変えると何が起きるのか、どういう設計にしたのか、これから入れる予定のFOVパンチ・ダッシュ中の視野角までをまとめます。

---

## 1. 視野角を変えると何が変わるか

DXライブラリの `SetupCamera_Perspective(角度)` は**縦方向**の視野角です。このゲームの標準は 60°（`DX_PI_F / 3.0f`）です。

視野角を変えるのは、カメラの位置は動かさずに**レンズを交換する**のと同じです。

- **狭くする（望遠）**：映る範囲が狭くなり、物が大きく映る。手前と奥の大きさの差が縮んで、画面が平たく見える
- **広くする（広角）**：映る範囲が広くなり、物が小さく映る。手前は大きく奥は小さくなり、遠近感・スピード感が強く出る

画面に映る物の大きさは `tan(30°) / tan(新しい視野角 / 2)` 倍になります。

| 視野角 | 物の大きさ（60°比） | 印象 |
|---|---|---|
| 30° | 約2.15倍 | 望遠。ズームインした演出 |
| 40° | 約1.59倍 | ぐっと寄る |
| 45° | 約1.39倍 | やや寄り |
| **60°** | **1.00倍** | **標準** |
| 70° | 約0.82倍 | 少し広い |
| 80° | 約0.69倍 | 広角。アクションゲームでよく使う |
| 100° | 約0.48倍 | 画面の端がかなり歪む |

**10°変えるだけで、物の大きさは2割前後変わります。** 思っているより強く効くので、少しずつ試すのが大事です。

---

## 2. 最初の問題：演出のあとに画面がカクっとなる

空中暗殺のカメラ（`AirAssasinCameraStart`）で、視野角を 60° から 40° にだんだん狭める演出を入れました。
ところが、次のカメラに切り替わる瞬間に画面がカクっとなりました。

原因は、各カメラが自分で `SetupCamera_Perspective` を直接呼んでいたことです。

- Start の `Exit()` で `SetupCamera_Perspective(60°)` を呼んでいたので、**1フレームで 40° → 60° に戻っていた**
- カメラの位置はブレンドでなめらかに動いているのに、映る範囲だけが一瞬で約1.6倍に広がっていた

ほかにも、今の作りには次の問題がありました。

- 視野角が CameraManager の外で勝手に変わるので、**ブレンドも引き継ぎもされない**
- 演出の途中で別のカメラに割り込まれると、**視野角が狭いまま残る**ことがある（PlayerFollowCamera は視野角を設定しないので戻らない）
- 同じ `kCameraViewAngle` が11ファイルにコピーされていた

---

## 3. 設計：視野角を「カメラの状態の一つ」にする

カメラの位置・注視点は、もともと次の流れで動いていました。

1. 各カメラが目標値（goal）を計算する
2. 基底クラス（`CameraStateBase`）がブレンドする
3. `CameraManager` が1か所で DxLib に反映する

視野角もこの流れに乗せることにしました。

### 3-1. 状態として持つ（CameraStateBase）

```cpp
static constexpr float kDefaultFov = DX_PI_F / 3.0f;//標準の視野角(60度)

float m_fov = kDefaultFov;//視野角//現在
float m_startFov = kDefaultFov;//視野角//初期
float m_goalFov = kDefaultFov;//視野角//目的
```

- 視野角を気にしないカメラは何もしなくてよい（自動で 60°）
- 演出したいカメラは **`m_goalFov` を変えるだけ**

### 3-2. 引き継ぐ（CameraData と ChangeState）

```cpp
//CameraManager::ChangeState
data.fov = m_currentState->GetFov();
...
m_currentState = newState;
m_currentState->SetStartFov(data.fov);//Enterより前に入れる
m_currentState->Enter(data);
```

`Enter` の前に入れるので、**各カメラの `Enter` は書き換えずに済みました**。

### 3-3. ブレンドする（UpdateBlend）

位置と同じ経過時間を使って、`m_startFov` から `m_goalFov` へ補間します。

```cpp
//視野角は位置とは別のイージング(カット切り替えのときはすぐに目標値にする)
if (m_activeBlend.mode == BlendSetting::Mode::None)
{
	m_fov = m_goalFov;
}
else
{
	float fovDuration = m_activeBlend.GetFovDuration();
	float tFov = (fovDuration > 0.0f) ? m_blendElapsed / fovDuration : 1.0f;
	tFov = std::clamp(tFov, 0.0f, 1.0f);
	tFov = Easing::Apply(m_activeBlend.fovEasingMode, tFov, m_activeBlend.fovEasingPower);
	m_fov = std::lerp(m_startFov, m_goalFov, tFov);
}
```

### 3-4. 反映は1か所だけ（CameraManager）

`CameraManager::Update` と `ApplyCameraSettings` で、毎フレーム `SetupCamera_Perspective(GetFov())` を呼びます。
**各カメラからは `SetupCamera_Perspective` を全部消しました。**

---

## 4. 視野角だけ別のイージング・長さにする

### なぜ別にするのか

位置のブレンドには `EaseOutBack`（目標を少し行き過ぎてから戻る）を使っているカメラがあります。
同じイージングを視野角にも使うと、**視野角も行き過ぎます**（60° → 63° → 60° のように）。

- 位置の行き過ぎ：カメラが回り込んで止まる動きとして自然に見える
- 視野角の行き過ぎ：**画面全体が膨らんで縮む**ように見えて、「揺れた」「カクついた」と感じやすい

市販のエンジン（Unreal の PlayerCameraManager、Unity の Cinemachine）も、カメラの切り替えでは視野角を位置と一緒に補間しています。
ただ「切り替えでは行き過ぎさせない」「わざと行き過ぎさせるのは演出（FOVパンチ）として別にやる」と分けたほうが扱いやすいので、視野角だけ別に設定できるようにしました。

### BlendSetting に足した項目

```cpp
struct BlendSetting
{
	...
	EasingMode fovEasingMode = EasingMode::EaseOut;//視野角のイージング(位置とは別)
	float fovEasingPower = 2.0f;//視野角のイージングの指数
	float fovDuration = -1.0f;//視野角を何フレームで目標にするか(負の値ならdurationと同じ)

	float GetFovDuration()const { return (fovDuration < 0.0f) ? duration : fovDuration; }
};
```

- **最後に足した**ので、今ある `BlendSetting{ .mode = ..., .duration = ... }` はそのまま通る
- 指定したいカメラでは、**一番最後に**書く（指示付き初期化子は宣言順に書く必要があるため）
- 書かなければ「EaseOut・2.0・位置と同じ長さ」。行き過ぎず、最後がゆっくり止まるので、どの位置の動きとも合う

### fovDuration と IsBlending

`IsBlending()` は、**位置と視野角の長いほうが終わるまで** true にしました。

```cpp
bool IsBlending()const { return m_blendElapsed < (std::max)(m_activeBlend.duration, m_activeBlend.GetFovDuration()) || ...; }
```

- 視野角のほうが長いときは、位置が着いていてもブレンド中扱いになる（位置は目標のままなので、見た目は変わらない）
- 「ブレンドが終わったら次のカメラへ移る」カメラ（AirAssasinCamera など）は、移るのがその分遅れる

### どのカメラの設定が使われるか（大事）

ブレンドの設定は、**切り替わった先のカメラの `m_activeBlend`** で決まります。

- 普通の切り替え：**新しいカメラ**の `GetBlendSetting()`
- 前のカメラが上書きブレンドを指定しているとき：**前のカメラ**の `GetOverrideBlendSetting()`

---

## 5. つまずきやすいところ

### Mode::None には2つの意味がある

| どこの None か | 意味 |
|---|---|
| `GetBlendSetting()` が返す None（＝`m_activeBlend.mode`） | **カット**（補間しない） |
| `GetOverrideBlendSetting()` が返す None（＝`data.overrideSetting.mode`） | **上書きの指定なし**（次のカメラは自分の `GetBlendSetting()` を使う） |

`UpdateBlend` で見ているのは前者です。カット切り替えのときは、位置と一緒に視野角も一気に切り替わります。
視野角だけゆっくり変わると、「位置はパッと変わったのに画面だけズームする」ちぐはぐな見え方になるためです。

### ブレンドが終わったあとの `m_fov`

各カメラの `else { m_pos = m_goalPos; m_target = m_goalTarget; }` に、`m_fov = m_goalFov;` は書いていません。
代わりに `GetFov()` が「ブレンドが終わっていたら `m_goalFov` を返す」ようにしています。

```cpp
float GetFov()const { return IsBlending() ? m_fov : m_goalFov; }
```

- `else` を持つカメラが10個以上あり、全部に書き足すと新しいカメラで書き忘れやすいため
- そのかわり、**カメラの中で今の視野角を使いたいときは `m_fov` ではなく `GetFov()` を使う**
- `BlendSetting` が決めるのは「ブレンド中の変わり方」だけ。ブレンド後の値は `GetFov()` が担当している

| | 担当しているところ |
|---|---|
| ブレンド中の変わり方（カーブ・長さ） | `BlendSetting` の `fovEasingMode` / `fovEasingPower` / `fovDuration` |
| ブレンド後の値 | `GetFov()` が `m_goalFov` を返す |

---

## 6. 使ってみた例：空中暗殺のカメラ

流れは地上の暗殺と同じです。

| タイミング | カメラ | 視野角 |
|---|---|---|
| 空中暗殺の開始（`PlayerStateAirAssasin::Enter`） | AirAssasinCameraStart | 60° → 40°（Start の `GetBlendSetting()`、45F） |
| 処刑が終わったとき（Execute → End） | AirAssasinCamera | 40° → 60°（Start の `GetOverrideBlendSetting()`、40F） |
| AirAssasinCamera のブレンドが終わったとき | PlayerFollowCamera | 60° のまま |

Start でやっていることは、`Enter` で `m_goalFov = 40°` にして、ブレンドの設定に視野角の項目を書くことだけです。

```cpp
BlendSetting AirAssasinCameraStart::GetBlendSetting() const
{
	return BlendSetting{
		.mode = BlendSetting::Mode::Lerp,
		.duration = kBlendDuration,
		.easingMode = EasingMode::EaseOut,
		.easingPower = kBlendEasingPower,
		.fovEasingMode = kFovEasingMode,
		.fovEasingPower = kFovEasingPower,
		.fovDuration = kFovDuration
	};
}
```

- ブレンドはタイムスケールに関係なく**実フレーム**で進む（スロー中も同じ速さで変わる）
- AirAssasinCamera は PlayerFollowCamera の初期値（距離350・縦 π/10）の位置へ戻り、着いたら PlayerFollowCamera に切り替わる（PartBrokenACameraEnd と同じ作り）

---

## 7. デバッグ：Camera Debug ウィンドウで視野角を調整する

フォトモード中の Camera Debug ウィンドウに、次を足しました。

- `Current FOV`：今の視野角（度）
- `FOV (deg)`：ドラッグで変更（10〜120°）
- `Reset FOV`：60° に戻す

フォトモード（ステージ編集中も）と `Override Camera` のときに使われます。Camera Animator の再生中は 60° のままです。

---

## 8. これから：FOVパンチ

### FOVパンチとは

攻撃が当たった瞬間などに、**視野角を一瞬だけ変えて、すぐ元に戻す**演出です（FOVキックとも呼ばれます）。

- **カメラシェイク**：画面が縦横に揺れる。ぶつかった・揺さぶられた感じ
- **FOVパンチ**：画面が奥や手前にぶれる。衝撃が広がった・吸い込まれた感じ

画面の位置はほとんどずれないので、シェイクより**見やすさを崩さずに強さを足せる**のが特徴です。
変化が大きいのは画面の端（視界の周辺）なので、目で追わなくても体で感じる手応えになります。

### 形と印象

| 種類 | 動き | 印象 | 向いている場面 |
|---|---|---|---|
| 広げる | 一瞬で広がり、ゆっくり戻る | 物が奥へ押しやられる。衝撃・圧が広がる・勢い | 重い一撃、爆発、着地、溜めの解放 |
| 狭める | 一瞬で狭まり、ゆっくり戻る | 物が迫ってくる。踏み込む・集中する | 溜め、発動の瞬間、とどめの直前 |
| バネ | 行き過ぎながら揺れて戻る | 弾む軽さ・気持ちよさ | アイテム取得、軽めの成功演出 |

コツは **「入りは一瞬（1〜3F）、戻りはゆっくり（10〜20F）」**。入りまで遅いと「ズームした」ように見えて、衝撃の感じが出ません。

### 強さの目安

- **2〜4°**：気づかないくらいの手応え。強攻撃向け
- **5〜8°**：はっきり分かる。スキル・とどめ向け
- **10°以上**：かなり派手。必殺技の最後の一撃などに絞る

### よく使われる場面

- 重攻撃・クリティカル・とどめが当たった瞬間（ヒットストップと一緒に使うことが多い）
- 自分が攻撃を受けた瞬間
- 溜めている間はじわっと狭め、放った瞬間に一気に広げる（狭めた分だけ広がりが強く感じられる）
- パリィ・ジャスト回避など、腕が決まった瞬間のご褒美

### このゲームで合いそうな場面

| 場面 | おすすめの形 |
|---|---|
| ジャスト回避・弾き（パリィ）の成功 | 狭める（時間が止まるような集中感） |
| ガードブレイク | 広げる（崩した衝撃） |
| 強攻撃・スキルの最後の一撃 | 広げる（小さめ） |
| 血殺の最後の一撃 | 狭めてから広げる（溜めと解放） |
| スキル3の着地の衝撃 | 広げる（地面から圧が広がる） |
| 空中暗殺の着地 | 広げる（小さめ） |

### 実装の方針（カメラシェイクと同じ「足す」形）

FOVパンチは「カメラを切り替える」ものではなく「今のカメラに一瞬だけ足す」ものなので、**ブレンドとは別に、CameraManager に持たせます**。
今のカメラシェイク（`StartCameraShake` / `CameraShakeUpdate`）と同じ作りです。カメラが切り替わってもパンチが途中で消えません。

```cpp
void StartFovPunch(float angleDeg, float time);//+で広げる、-で狭める
float FovPunchUpdate();//今フレームのずれ(ラジアン)。Updateで1回だけ呼ぶ

//CameraManager::Update
m_renderFov = m_currentState->GetFov() + FovPunchUpdate();
SetupCamera_Perspective(m_renderFov);
```

- `FovPunchUpdate()` は **`Update` で1回だけ**呼び、結果を `m_renderFov` に覚える（シェイクの `m_renderPos` と同じ）。`ApplyCameraSettings` でも呼ぶとタイマーが2回減る
- 多段ヒットで何度も呼ばれても大きくなりすぎないように、**足し算ではなく「今より強いときだけ上書き」**
- 入りと戻りでカーブを分ける（シェイクはまっすぐ弱まるだけだが、パンチは入りが遅いとズームに見える）
- 呼ぶ場所はシェイクと同じ `AttackCol` の当たったところ。最初は 4°・14F くらいで試す

---

## 9. これから：ダッシュ・ブーストで視野角を広げたままにする

### やりたいこと

ダッシュやブーストの間は、視野角を**広げたままにしておきたい**。
タイムスケール（`System::SetTimeScale`）のように、「設定したら、戻すまでずっとその値」にします。

FOVパンチとの違いは次のとおりです。

| | FOVパンチ | ダッシュ・ブースト |
|---|---|---|
| 長さ | 一瞬（自動で戻る） | 戻すまでずっと |
| 入り方 | 一瞬で変わる | なめらかに広がる |
| 戻り方 | 自動でゆっくり戻る | 戻すときに指定した長さで戻る |

### 設計案

これも CameraManager に持たせ、FOVパンチと同じく「足す」形にします。

```cpp
//戻すまでずっと視野角に足す(+で広げる)。blendFrameかけてなめらかに変える
void SetFovOffset(float angleDeg, float blendFrame);
//元に戻す(SetFovOffset(0, blendFrame)と同じ)
void ResetFovOffset(float blendFrame);
//戻るまでのフレーム数を指定する版(SetTimeScaleForFramesと同じ考え方)
void SetFovOffsetForFrames(float angleDeg, float blendFrame, int frames);
```

- 変えるときは、**今の値から新しい値へ `blendFrame` かけてイージング**する。途中で別の値に変えられても、そのときの値から続けて変わるのでカクつかない
- 最終的な視野角は次の足し算にする

```cpp
m_renderFov = m_currentState->GetFov()  //カメラごとの視野角(ブレンド込み)
            + FovOffsetUpdate()         //ダッシュ・ブースト(戻すまでずっと)
            + FovPunchUpdate();         //FOVパンチ(一瞬)
m_renderFov = std::clamp(m_renderFov, kFovMin, kFovMax);//10〜120度くらい
```

- 使い方の例：`PlayerStateFastRun::Enter` で `SetFovOffset(8, 12)`、`Exit` で `ResetFovOffset(15)`
  - **戻し忘れ防止のため、必ず Exit で戻す**（Enter と Exit で対にする）

### 決めておくこと

1. **演出カメラ中も足すか**：暗殺・必殺技・血殺のカメラは決めた構図で見せたいので、足さないほうがよさそう。
   案：`CameraStateBase` に `virtual bool IsUseFovEffect()const { return true; }` を作り、演出カメラだけ false を返す。false のカメラ中はオフセットとパンチを足さない
2. **タイムスケールを掛けるか**：シェイクはタイムスケールを掛けている。ダッシュの広がり方はスローの影響を受けないほうが自然なので、実フレームがよさそう。パンチは「スロー中にすぐ戻ってほしいか」で決める
3. **誰が設定したかの管理**：今の案は「最後に設定した人が勝ち」（タイムスケールと同じ）。ダッシュ中にブーストが重なるなど、複数が同時に設定したくなったら、名前付きで持って一番大きいものを使う形に変える

---

## 10. 新しくカメラを作るときのルール

- **`SetupCamera_Perspective` を直接呼ばない**（CameraManager が毎フレーム上書きするので効かない）
- 視野角を変えたいときは **`m_goalFov` を変える**。変わり方は `BlendSetting` の `fovEasingMode` / `fovEasingPower` / `fovDuration` で決める
- カメラの中で今の視野角を使うときは **`GetFov()`** を使う（`m_fov` はブレンド中しか正しくない）
- ニア・ファー（`SetCameraNearFar`）は今もカメラごとに設定している
