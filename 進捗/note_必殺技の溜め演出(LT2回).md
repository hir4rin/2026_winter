# 必殺技(LT2回)の構え〜溜めの演出 — コードの解説

2026-11-21〜22 の作業。NINJA GAIDEN 4 のヤクモの真血殺のように、LT2回の専用必殺技(`PlayerStateUlt`)の**構え〜溜め**にカメラワークとエフェクトを付けた。
**当たった後の血殺の演出(`UltCamera`・色変え・白い血・文字)は今のまま**で、触っていない。

リンクはこのファイル(`進捗/`)からの相対パス。行番号は 11/22 時点。

---

## 0. 全体の流れ

| 段階 | 長さ | カメラ | 刀 | 血のエフェクト |
|---|---|---|---|---|
| 構え (Start) | 約19F | 前へ回り込む(Slerp 12F・画角45°) | 普通の刀の長さ | 吸い込み開始 |
| 溜め前半 (Loop 0〜22F) | 22F | 正面のローアングル | 普通の刀の長さ | 床から血の筋が集まる |
| 溜め後半 (Loop 22〜58F) | 36F | 右へ回りながら引く | 刀身だけ伸びる | 約60Fで終わる |
| 振り (Attack) | 〜当たるまで | 18Fで後ろへ戻る → いつものカメラ | 長い刀 | (今までの斬撃) |
| 当たった後 | 血殺の演出 | **今のまま** UltCamera | 長い刀 | **今のまま** |

関わるファイル:

| 役割 | ファイル |
|---|---|
| 流れ(時間の管理) | [PlayerStateUlt.cpp](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp) / [.h](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.h) |
| 溜め用のカメラ(新規) | [UltChargeCamera.cpp](../2026_winter/Camera/CameraState/UltChargeCamera.cpp) / [.h](../2026_winter/Camera/CameraState/UltChargeCamera.h) |
| 刀を伸ばす | [PlayerWeapon.cpp](../2026_winter/Character/Player/PlayerWeapon.cpp) / [.h](../2026_winter/Character/Player/PlayerWeapon.h) |
| カメラが刀の伸び具合を読む窓口 | [Player.cpp:1163](../2026_winter/Character/Player/Player.cpp#L1163) / [Player.h:290](../2026_winter/Character/Player/Player.h#L290) |
| 隠したメッシュを描かない | [ModelShaderManager.cpp:348](../2026_winter/Render/ModelShaderManager.cpp#L348) |
| カメラの登録 | [CameraStateBase.h:89](../2026_winter/Camera/CameraState/CameraStateBase.h#L89)、[CameraManager.h:46](../2026_winter/Camera/CameraManager.h#L46)、[CameraManager.cpp:265](../2026_winter/Camera/CameraManager.cpp#L265)・[:704](../2026_winter/Camera/CameraManager.cpp#L704)、vcxproj/filters |
| エフェクトの読み込み | [System.h:84](../2026_winter/Main/System.h#L84)、[System.cpp:65](../2026_winter/Main/System.cpp#L65)・[:115](../2026_winter/Main/System.cpp#L115)・[:310](../2026_winter/Main/System.cpp#L310) |
| エフェクト本体 | `data/Effect/Blood/UltBloodAbsorb.efk`(作業ファイル `UltBloodAbsorb.efkefc`)、`data/Effect/Blood/Texture/UBA_Glow.png` |

---

## 1. 流れの管理 — `PlayerStateUlt`

もともと `PlayerStateUlt` は `Phase::Start → Loop → Attack` の3段階で進む作りだった。**段階の切り替えはそのまま**にして、各段階に処理を足した。

### 1-1. Enter で3つを始める — [PlayerStateUlt.cpp:57](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L57)

```cpp
player->m_weapon->SetUltWeapon(true);
player->m_weapon->SetUltBladeRate(0.0f);              // 刀を普通の長さにしておく
m_absorbHandle = EffectManager::GetInstance().Play(   // 血の吸い込みを刀の根元に出す
    AsyncData::UltBloodAbsorbEffect, base, 0.0f, kAbsorbEffectScale);
cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::UltChargeCamera);
```

- **なぜ Enter で全部始めるか**: 構え(Start)は約19Fと短く、ここからカメラが回り込み、血も集まり始めないと溜めに間に合わないため。
- `SetUltWeapon(true)` は伸び具合を1に戻すので、**その後に** `SetUltBladeRate(0)` を呼ぶ順番にしている。
- エフェクトは `EffectManager::Play` で出す。プレイヤーの他のエフェクトは `PlayEffekseer3DEffect` を直接呼んでいるものもあるが、`EffectManager` 経由にしておくと、タイムスケールや後片付けの扱いが他のエフェクトと揃う。

### 1-2. 吸い込みの中心を刀の根元に合わせ続ける — [PlayerStateUlt.cpp:116](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L116)(Update の先頭)

毎フレーム `GetBladePos` の根元(base)へ `SetPosPlayingEffekseer3DEffect` する。再生が終わったらハンドルを -1 にして触らない。

- **なぜ毎フレームか**: 構えのアニメで手(=刀の根元)が動くので、出した位置に置いたままだと血が何もない所へ吸い込まれて見えるため。
- エフェクト側は子ノードが親についていく設定(LocationEffectType = Already)なので、中心を動かせば筋の行き先も一緒に動く。

### 1-3. 溜めの間に刀を伸ばす — [UpdateBladeGrow (PlayerStateUlt.cpp:223)](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L223)

定数は [PlayerStateUlt.cpp:41-45](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L41)。

```cpp
float t = clamp((frame - kBladeGrowStart) / (kBladeGrowEnd - kBladeGrowStart), 0, 1);  // 22F〜58F
t = t * t * (3 - 2 * t);                    // smoothstep: ゆっくり出てゆっくり止まる
player->m_weapon->SetUltBladeRate(t);
```

- **なぜ Loop の 22F から**: 前半の22Fは「血を吸い込む」時間として空けておき、吸い込んだ血で刀が伸びる、という順番に見せるため。Loop は60Fなので、58Fで伸びきって最後の2Fは止める(振る前の「ため」)。
- **なぜ smoothstep か**: 一定の速さで伸びると機械的に見える。出だしと止まり際をゆるめると、力をためて伸びるように見える。
- 伸び始めた最初のフレームだけ `StartCameraShake(2, 36)` で軽く揺らす(`m_isBladeGrowStarted` で1回だけ)。「何かが起きた」合図。

**大事な点**: ここで決めた「伸び具合(0〜1)」を、**カメラもそのまま読んで位置を決める**(→ 2-3)。カメラ側で別にタイマーを持たないので、刀の伸びとカメラの引きが必ず揃う。時間を変えたいときは `kBladeGrowStart / End` だけ直せばよい。

### 1-4. 振りに入ったら戻す — [ChangePhase (PlayerStateUlt.cpp:90)](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L90)

```cpp
case Phase::Attack:
    player->m_weapon->SetUltBladeRate(1.0f);  // 念のため伸びきった状態にする
    ReturnChargeCamera();                       // 溜めのカメラなら後ろへ戻り始める
```

- [ReturnChargeCamera (PlayerStateUlt.cpp:243)](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L243) は、今のカメラが `UltChargeCamera` のときだけ `StartReturn()` を呼ぶ(`dynamic_pointer_cast` で確認)。
- **なぜ「戻す」のが振りの始まりか**: 当たり判定はアニメの23〜30F。当たると `AttackCol` が `UltCamera` に切り替える(既存の処理)。その前にいつものカメラ寄りへ戻しておけば、当たったときの `UltCamera` への切り替えも、空振りしたときの終わり方も、今までと同じになる。
- [Exit (PlayerStateUlt.cpp:206)](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L206) でも `ReturnChargeCamera()` を呼ぶ。途中で別のステートに抜けたとき、溜めのカメラが残り続けないようにするための保険。

---

## 2. 溜め用のカメラ — `UltChargeCamera`(新規)

[UltChargeCamera.cpp](../2026_winter/Camera/CameraState/UltChargeCamera.cpp)。値はすべて先頭([:9-40](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L9))にある。

### 2-1. なぜ新しいステートにしたか

- 既存のカメラは `CameraStateBase` を継承したステートで、`CameraManager::ChangeState` で切り替える作り。ブレンド・視野角の引き継ぎ・ステージとの当たり判定などが全部この仕組みに乗っているので、同じ形にした。
- 案の段階では「カット割り(案A)」も検討したが、ユーザーの希望(プレイヤーの前から、血を吸って刀が伸びるのを見せる)に合わせて、**1本の動きで寄る→引く**形にした(会話の「案2」)。

登録した所: `Type::UltChargeCamera = 31`([CameraStateBase.h:89](../2026_winter/Camera/CameraState/CameraStateBase.h#L89))、`CameraStateName::UltChargeCamera`([CameraManager.h:46](../2026_winter/Camera/CameraManager.h#L46))、`ChangeStateFromScene` の case([CameraManager.cpp:704](../2026_winter/Camera/CameraManager.cpp#L704))、デバッグ表示の名前([CameraManager.cpp:265](../2026_winter/Camera/CameraManager.cpp#L265))。

### 2-2. Enter — プレイヤーの「前」を決める — [UltChargeCamera.cpp:51](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L51)

- `player->GetTargetVec()`(水平にして正規化)を「前」(`m_front`)として**Enter で1回だけ**決める。
  - **なぜ `m_targetVec` か**: `PlayerStateUlt::Enter` の `DecideAttackDirection()` で攻撃の向き(入力→ロックオン→内部ターゲット)が入っている。モデルの向き(`m_rotAngleY`)は1割ずつしか回らないので、Enter の瞬間はまだ古い向きのことがある。
  - **なぜ1回だけか**: 毎フレーム取り直すと、構えのアニメ中に向きが変わったときにカメラが振り回される。
- 前へ回り込むブレンドは `GetBlendSetting`([:189](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L189))で **Slerp 12F・EaseOut・中心はプレイヤー**。
  - **なぜ Slerp か**: いつものカメラはプレイヤーの後ろ、行き先は前。Lerp(直線)だとプレイヤーの体を突き抜ける。プレイヤーを中心に回り込ませれば体を避けられる(`BossPartThrowCamera` と同じやり方)。
- 視野角は `m_goalFov` を 45° にするだけ。ブレンド中の視野角の補間は `CameraStateBase` がやってくれる(`CLAUDE.md` の「視野角をカメラの状態として扱う」)。

### 2-3. 溜めの位置 — 刀の伸び具合で決める — [CalcChargePos (:147)](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L147)

```cpp
const float rate = player->GetUltBladeRate();           // 刀の伸び具合 0〜1
dist   = lerp(230, 300, rate);   // プレイヤーからの水平距離
height = lerp( 40,  75, rate);   // カメラの高さ(低い=見上げる)
targetHeight = lerp(115, 95, rate);
fov    = lerp(45°, 55°, rate);
side   = lerp(15°, 40°, rate);   // 正面からプレイヤーの右へ回す角度
target += right * (50 * rate);   // 注視点を刀の側へずらす
```

- **なぜ刀の伸び具合を読むか**: 1-3 のとおり、時間の管理を `PlayerStateUlt` の1か所にまとめるため。刀の伸びとカメラの引きがずれることがない。
- **伸びる前(rate=0)**: 正面・距離230・高さ40の**ローアングル**、画角45°の望遠気味。顔と構えを大きく、見上げて強く見せる。
- **伸びた後(rate=1)**: 右へ40°回り、距離300・高さ75、画角55°。
- **なぜ右へ回るのか(大事)**: 最初はモックどおり真正面から引く形にしたが、ゲームで撮ると刀が見えなかった。
  - `Power_Attack_Loop` は**刀を逆手に持ち、足の後ろの斜め下へ構える**姿勢だった。一時コードで刀の先の位置を記録すると、伸びきった刀の先は足元から**右へ約160・後ろへ約130・下へ約50**。
  - 真正面からだと刀が奥へ向かって縮んで見え、さらに足に隠れて画面の下に切れていた。
  - 刀が伸びる向き(右後ろ)に対してほぼ直角になる位置(右へ40°)から見ると、刀身が横に長く見える。計算でも、カメラの視線と刀の向きのなす角は約83°(ほぼ真横)。
  - 注視点も刀の側へ50ずらし、プレイヤーと伸びた刀の両方が画面に収まるようにした。
- 小さな手持ちの揺れ(1.5、sin)を足している。完全に止まったカメラより「撮っている」感じが出る。

### 2-4. 戻り — いつものカメラの位置へ回り込む — [StartReturn (:137)](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L137) / Update の戻りの部分 ([:73](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L73))

- `StartReturn()` で今の位置・注視点・視野角を覚え、18F かけて `PlayerFollowCamera` の初期位置([CalcFollowPos (:174)](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L174)。プレイヤーの後ろ・距離350・縦 π/10・注視点の高さ150)へ動かす。着いたら `PlayerFollowCamera` に切り替える。
- 動かし方は**プレイヤーの足元を中心に「水平の角度・水平距離・高さ」をそれぞれ補間**する。角度は差を -π〜π に直して近いほうへ回す。
  - **なぜ自分で補間するか**: カメラの仕組みの「上書きブレンド(`GetOverrideBlendSetting`)」で `PlayerFollowCamera` に任せる手もあるが、`PlayerFollowCamera::Enter` は上書きブレンドのときに**渡した角度を使わない**作りだった。そうすると戻った後の向きがおかしくなる。上書きなしだと `PlayerFollowCamera` は Lerp(直線)なので体を突き抜ける。そこで `AirAssasinCamera` と同じく「自分で行き先まで動かしてから切り替える」形にした。行き先と `PlayerFollowCamera` の初期位置が同じなので、切り替えた瞬間に飛ばない。
- [Exit (:130)](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L130) で `m_angleH = atan2(前)`、`m_angleV = π/10` を入れる。`PlayerFollowCamera` はこの角度を引き継ぐので、プレイヤーの後ろから前を見る向きで始まる。
- 戻っている途中で攻撃が当たった場合は、`AttackCol` が `UltCamera` に切り替える。`UltCamera` は今の位置から Slerp でブレンドするので、途中からでも自然につながる。

### 2-5. 入れていないもの

- `IsFovOffsetCamera` には入れていない。ボス戦の視野角 +20° は溜めの間だけ効かなくなる(出入りは10Fで切り替わるので画面は飛ばない)。
- ステージとの当たり判定は `CameraManager` の今の設定のまま。前から低く見上げるので、**壁ぎわでカメラが壁に入らないかはまだ見ていない**。

---

## 3. 刀を伸ばす — `PlayerWeapon`

### 3-1. やり方を決めた理由

長い刀(`red_katana_ult.mv1`)は普通の刀の約3.3倍の長さ。案は3つあった。

| 案 | 問題 |
|---|---|
| 普通の刀→長い刀へ一瞬で持ち替え(元の作り) | 「伸びていく」のが見えない |
| 長い刀全体を刀の向きに拡大縮小 | 柄・鍔まで縮んで潰れる(伸び具合0で柄が約1/4になる) |
| **刀身のメッシュだけを、鍔を中心に刀の向きへ伸ばす** | 柄はそのまま、刀身だけが伸びる → これにした |

長い刀のモデルは10メッシュで、そのうち刀身は2つ(2番と4番)。

### 3-2. 刀身と鍔を自動で調べる — [SetupUltBlade (PlayerWeapon.cpp:160)](../2026_winter/Character/Player/PlayerWeapon.cpp#L160)

コンストラクタで1回呼ぶ。

1. 刀の先(`m_ultTipLocal`、既存の `CalcTipLocal` が求めたもの)から、**刀の向きの軸**(x/y/z のうち一番長い軸)と向き(+/-)を決める。このモデルは Y 軸。
2. 各メッシュの範囲を**フレームの行列を掛けてモデルの空間に直して**から、刀の先の半分より遠くまで届くメッシュを「刀身」とする(`m_isUltBladeMesh`)。
3. 刀身のメッシュのうち、柄の側の端(=鍔の位置)を `m_ultPivot` にする。柄の中まで通っているメッシュ(4番は柄の中まで芯が通っている)は鍔の位置の候補から外す。
4. 伸び具合0のときの倍率 `m_ultStartScale` = (普通の刀の鍔から先の長さ)÷(長い刀の鍔から先の長さ)= **約0.265**。伸び具合0で普通の刀と同じ長さに見える。

**つまずいた所(大事)**: 最初はメッシュの範囲(`MV1GetMeshMinPosition / MaxPosition`)をそのまま刀の先と比べていた。しかしこの値は**フレームの空間**で、このモデルのフレームには回転と×100の拡大が入っていた(メッシュの値は -5.26、刀の先は 525)。そのため刀身が1つも見つからず、**伸ばしている途中も長い刀がそのまま全部描かれていた**。フレームの行列を8つの角に掛けてから比べるように直した(既存の `CalcTipLocal` と同じやり方)。

> `MV1GetMeshMin/MaxPosition` の値を他の座標と比べるときは、フレームの行列を掛けること。

### 3-3. 伸ばす行列 — [CalcBladeStretchMatrix (:226)](../2026_winter/Character/Player/PlayerWeapon.cpp#L226) / [GetBladeStretchScale (:221)](../2026_winter/Character/Player/PlayerWeapon.cpp#L221)

```cpp
scale = startScale + (1 - startScale) * rate;          // 0.265 → 1
stretch = Translate(-鍔) × Scale(刀の軸だけ scale) × Translate(鍔);
```

- 鍔を原点に移し、刀の向きの軸だけを拡大し、元の位置に戻す。これで**鍔が動かずに刀身だけが伸びる**。
- DxLib の `MMult(a, b)` は「a を先に掛けてから b」(行ベクトル)なので、この順番で掛ける。描くときは `stretch × 刀の行列(CalcWeaponMatrix)` の順にする。

### 3-4. 描き方 — 2回に分けて描く — [DrawStretchedUlt (:238)](../2026_winter/Character/Player/PlayerWeapon.cpp#L238) / [Draw (:142)](../2026_winter/Character/Player/PlayerWeapon.cpp#L142)

伸び具合が1未満のときだけこの描き方になる。

1. 刀身のメッシュを隠し、いつもの行列で描く → **柄・鍔だけ**が描かれる。
2. 刀身のメッシュだけを見せ、`stretch × いつもの行列` で描く → **伸ばした刀身**が描かれる。
3. 全部のメッシュを見せ、行列をいつものものに戻す。

- **なぜ1つのモデルで2回描くか**: 刀身だけ別のモデルに分ける必要がない。どちらも `ModelShaderManager::DrawModel` を通るので、ステージの明かり・色変え(血殺の赤)なども今まで通り効く。
- 伸び具合が1(普段・振りの最中・血殺の最中)は今まで通り1回で描くので、普段の重さは変わらない。

### 3-5. 刀の先の位置も伸ばした先にする — [GetBladePos (:98)](../2026_winter/Character/Player/PlayerWeapon.cpp#L98)

伸ばしている途中は、刀の先に `CalcBladeStretchMatrix()` を掛けてから返す。

- **なぜ必要か**: 刀の軌跡(`m_swordTrail`)や、血の吸い込みの中心(根元)がこの値を使う。描いている刀と位置がずれないようにするため。

### 3-6. 公開した関数 — [PlayerWeapon.h:21-24](../2026_winter/Character/Player/PlayerWeapon.h#L21)

- `SetUltBladeRate(0〜1)` / `GetUltBladeRate()`。
- `SetUltWeapon()` は伸び具合を1に戻すようにした。必殺技以外で長い刀を持ったとき(今は無いが)に、途中の長さのまま残らないように。
- カメラから読むための窓口として `Player::GetUltBladeRate()`([Player.cpp:1163](../2026_winter/Character/Player/Player.cpp#L1163))を足した。`m_weapon` は private で、`PlayerStateUlt` は friend なので直接触れるが、カメラは friend ではないため。

---

## 4. 隠したメッシュを描かない — `ModelShaderManager`

[ModelShaderManager.cpp:348](../2026_winter/Render/ModelShaderManager.cpp#L348)

`DrawModelWithShader` は、モデルの中に頂点の種類が混ざっているときだけ、メッシュごとに `MV1DrawMesh` で描く。その中に `MV1GetMeshVisible` が FALSE なら飛ばす1行を足した。

- **なぜ**: 3-4 の2回描きはメッシュの表示・非表示で切り分けている。`MV1DrawMesh` で1つずつ描くと、表示の設定に関係なく描かれるおそれがあるため。`CharacterLook::Draw` でも同じ判定をしている。
- 正直に書くと、長い刀が全部描かれていた本当の原因は 3-2 の「刀身が見つからなかった」ほうで、この1行ではなかった。ただ、隠したメッシュを描かないのは正しい動きなので残してある。

---

## 5. 血を吸い込むエフェクト — `UltBloodAbsorb.efk`

Effekseer MCP で作った。`data/Effect/Blood/UltBloodAbsorb.efk`(作業ファイルは同じフォルダの `.efkefc`)。

### 5-1. 作り

| ノード | 種類 | 中身 |
|---|---|---|
| Stream | None | 64本を0.55Fおきに出す。寿命20〜28F。**Easing で、周り(半径約20)から中間点を通って原点へ**。始点は低速・終点は高速(加速しながら吸い込まれる) |
| Stream/Body | Track | 筋の尾。BloodHit の糸と同じく、左右を暗い赤・中央を明るい赤にして筒のように見せる。テクスチャは `BH_Strand_c` |
| Stream/Head | Sprite | 筋の先の粒。`BH_Fine_c` |
| Core | Sprite(加算) | 原点の赤い光。50Fかけて大きく・明るくなる。`UBA_Glow.png`(新しく作った丸い光) |

- **1/20 の大きさで作って、ゲームでは20倍で出す**(`kAbsorbEffectScale`)。他の血のエフェクトと同じ決まり。
- **床から集まるようにした理由**: 最初は空中から集まる形にしたが、血は床に流れているもの、という見え方のほうが自然。始点の高さを根元から下へ2〜5(×20=40〜100)にした。刀の根元は床から約75〜110の高さなので、だいたい床の高さから出る。
- **Easing を使った理由**: 「出た位置から原点へ」を確実に向かわせるには、速度(PVA)より始点と終点を決める Easing が簡単。中間点を乱数にすると、筋がまっすぐでなく少しうねって集まる。

### 5-2. ゲームで見て直したこと

- **Track の幅も20倍になる**。エディタでちょうどよかった太さ(中央0.7)がゲームでは画面を覆うほど太かったので、約1/3(中央0.22)にした。
- 粒(Head)と芯(Core)も小さくした。

### 5-3. 読み込み

`AsyncData::UltBloodAbsorbEffect`([System.h:84](../2026_winter/Main/System.h#L84))。`System` の起動時の読み込み(`LoadCommon`)で読む([System.cpp:115](../2026_winter/Main/System.cpp#L115))。

- **なぜ起動時か**: プレイヤーの必殺技はどのステージでも出せるので、ステージごとに読む敵のデータではなく、プレイヤーと同じ共通の読み込みに入れた。

---

## 6. 試して元に戻したもの(コードは残っていない)

### 6-1. 血殺の演出中にプレイヤーをカメラの方へ振り向かせる

- 振りの終わり際に `m_targetVec` をちょうど反対にした。しかし `UltCamera` は敵→プレイヤーの線から60°ずれた所にいるうえ、振りのアニメの終わりで体がひねられているので、回りきると**背中が見えた**。
- カメラの方へ向ける形にも直したが、見た目が変だったので取りやめ。
- 分かったこと: 向きの回転(`Player::UpdateRotAngle`)はタイムスケールに関係なく1割ずつ回るので、血殺のスロー(0.1)中でも約10〜20Fで振り向く。一方アニメはスローの影響を受け、当たった後は1フレームに0.05しか進まない。

### 6-2. 刀が伸びる間に刀身を光らせる

- 刀身に沿った加算の帯(外側の赤・芯の明るい色)と、刀の先の光を描いた。取りやめ。
- 分かったこと: `DrawPolygon3D` で描いた帯は**背面カリングで片面が消えていた**(刀の先の光(ビルボード)は出ていたので気づけた)。作り直すなら両面を描くこと。

---

## 7. 確かめたこと・まだのこと

確かめたこと:
- Debug・Release(x64)ともビルドが通る。
- 一時コード(ベンチで起動して120F目に必殺技を出し、5Fごとに画面を保存)で、デモステージで「前から見上げる → 血が集まる → 刀が伸びて横から見える → 後ろへ戻る」を撮って確認した。一時コードは消してある。

まだのこと:
- 実際に LT を2回押しての確認。
- 敵がいる所、暗いステージ、**壁ぎわ(前から見上げるカメラが壁に入らないか)**。
- 伸びきった刀の先が床に少し埋まる(アニメの姿勢のため)。

前からある件(今回は触っていない):
- 振りの始めのスロー(0.2)が、アニメの1.58Fですぐ1.0に戻っていて、ほぼ効いていない([UpdateAttack (PlayerStateUlt.cpp:163)](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L163))。

---

## 8. 調整したいときの早見表

| 変えたいこと | 場所 |
|---|---|
| 刀が伸び始める・伸びきる時間 | `kBladeGrowStart` / `kBladeGrowEnd`([PlayerStateUlt.cpp:41](../2026_winter/Character/Player/State/Attack/PlayerStateUlt.cpp#L41)) |
| 伸び始めの揺れ | `kBladeGrowShakePower` / `kBladeGrowShakeTime`(同上) |
| 吸い込みのエフェクトの大きさ | `kAbsorbEffectScale`(同上) |
| 溜めのカメラの距離・高さ・画角・回る角度 | `kNear〜` / `kFar〜`([UltChargeCamera.cpp:12-23](../2026_winter/Camera/CameraState/UltChargeCamera.cpp#L12)) |
| 手持ちの揺れ | `kSwayPower` / `kSwaySpeed`(同 :26) |
| 前へ回り込む速さ | `kBlendDuration` / `kBlendEasingPower`(同 :30) |
| 後ろへ戻る速さ | `kReturnFrame` / `kReturnEasingPower`(同 :34) |
| 血の筋の数・太さ・出る範囲 | `UltBloodAbsorb.efkefc` を Effekseer で開いて直し、同じ名前で `.efk` に書き出す(Track の幅は20倍になることに注意) |
