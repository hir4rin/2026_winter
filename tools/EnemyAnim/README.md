# EnemyAnim

敵(Swordman の `Enemy.mv1`、Gunner・Bazooka の `EnemyGun.mv1`)にアニメを足す手順とツール。
メッシュ・マテリアルは今の mv1 のまま、アニメだけ Unity から書き出し直したものに入れ替える(`tools/PlayerAnim` と同じ考え方)。

## 今入っている足したアニメ

| mv1 のアニメ名 | アニメCSVの key | 中身 |
|---|---|---|
| `Enemy_Launch` | `Launch` | Mixamo の `Female Dynamic Pose`(体が二つ折りで手足が垂れる1枚のポーズ)から作った40F、ループなし。0Fで強く折れる → 4Fで少し戻りすぎる → 9Fでポーズ → 40Fにかけて少しほどけて浮く。足首は力の抜けた角度(`kFootRelax`)、膝は少し曲げる |
| `Jump_Combat_Loop_0` | `AirFall` | **プレイヤーの空中(JumpDown)と同じアニメ**。`SwordAnimationPack/Animation/Humanoid/.../Jump_Combat_Loop_0.anim` をそのまま書き出す |

`Enemy_Launch` は Unity の `ninja_motion/Assets/Editor/EnemyAnimTool.cs` で Humanoid の筋肉の値を書いて作る(`Assets/EnemyAnim/Enemy_Launch.anim`)。動きを変えるときは `LaunchPose` の数字を変える。

## 手順

Unity のエディタが閉じていればバッチで動く(開いているなら `Tools > Enemy Anim` のメニュー)。結果は `ninja_motion/Library/EnemyAnimTool.result`。

1. `Setup`: `Downloads/Female Dynamic Pose.fbx` を `Assets/EnemyAnim/FemaleDynamicPose.fbx` にコピーして Humanoid にする(済み)
2. `MakeClips`: `Enemy_Launch` を作る。`ShotMade` で `Library/EnemyAnimShots/` に撮る(Swordman の体、上段=斜め前・下段=横)
3. **`run_all.sh 作業フォルダ`**(3〜6をまとめてする。書き出しを飛ばすなら2つ目の引数に `noexport`)
   - 作業フォルダには `DxLibModelViewer_64bit.exe` のコピーと `Setting.ini`(`LoadAndSave=1`、ウィンドウは左のモニター `WindowX=-1880`)、前の mv1 を `Enemy_old.mv1` / `EnemyGun_old.mv1` の名前で置く
   - `EnemyAnimTool.Export`: SampleScene の `sasakiPlayer`(TekiHidameAnim.controller)と `Reloading`(GunTekiAnim.controller)を、今のアニメ＋足したアニメで `Library/EnemyAnimExport/Swordman.fbx` / `Gunner.fbx` に書き出す。
     今の mv1 と同じ設定(FPS 30、腰 `mixamorig:Hips` の XZ を固定、Y は残す)。Swordman は Mixamo の9本の名前を付け替える(`Falling Back Death`→`Die` など。`SwordmanNames`)
   - ビューアで mv1 にする → `animmerge`(圧縮なし)で前の mv1 にアニメだけ入れ替え → `animcmp` で今のアニメと比べる。**差が0.5以上か名前が欠けていたら変換し直す**(5回まで)。通ったものだけ `data/Enemy/` に上書き
4. アニメCSVに key を足す(Bazooka は EnemyGun.mv1 を使う)

## 気をつけること

- **ビューアの変換は同じ FBX でも毎回結果が違い、ときどき壊れる**(ある骨が 10の13乗 のような値になる、指先が数cmずれる)。読み込みはできるので気づきにくい。必ず `animcmp` で前のアニメと比べる。
  Gunner は5回続けて `Gunplay` が崩れたことがある(そのときは前に正常だった変換を使った)。
- **筋肉の値だけで作ったクリップは足の目標(`LeftFootT` など)を持たない**。足IKを掛けて書き出すと足が1点に引っ張られる(最初の版はこれで足が壊れていた)。
  `SkinnedAnimFbxExporter.UseFootIK`: 目標の無い `.anim` だけ足IKを切る。FBX のクリップは今まで通り掛ける(`Gunplay (1)` は目標が無いが、切ると前の mv1 と変わるため)。`EnemyAnimTool` の撮影も同じ決まり。
- DxLib(ゲームのライブラリ)は FBX を読めない(-1)。変換はビューアで行う。
- 敵の体が変わったら(SampleScene の敵を作り直したら)、骨の並びが今の mv1 と同じか animmerge のログ(`frame differs`)で見る。

## ツール

- `animcmp.exe ログ 前のmv1 新しいmv1`: 同じ名前のアニメを全フレーム・全骨の位置で比べる(B を省くと腰の動きだけ)
- `bones.exe ログ モデル アニメ名 フレーム...`: 主な骨(腰・頭・脚・手)の位置
- `inspect.exe ログ モデル...`: 骨・アニメの一覧
- `shot.exe 出力.png モデル アニメ名 枚数`: 時間を並べて4方向(前・横・後ろ・横)から撮る(DxLib の標準の描画、ウィンドウは出さない)
- `animmerge.exe 今のmv1 アニメのmv1 出力.mv1 ログ`: `tools/PlayerAnim` と同じで、圧縮せずに保存する
- ビルドは `build.bat 名前`(例 `build.bat shot`)
