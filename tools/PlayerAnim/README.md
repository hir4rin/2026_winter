# PlayerAnim

`Player03.mv1` にアニメを足す手順。

`Player03.mv1` を FBX から丸ごと作り直すと、髪の削減(`tools/HairLod`)・マテリアルの色・縮めたテクスチャの設定が消える。
そのため、FBX からはアニメだけを取り、メッシュは今の `Player03.mv1` のまま保存する。

## 手順

1. Unity(`ninja_motion`)で `Assets/TruePlayer.controller` に足したいクリップを追加する。ここにあるクリップが書き出される。
2. `Assets/NInjaGirl/IdaFaber/Meshes/Girl/SK_VEX_01.fbx` を一時シーンに置いて、Animator に `TruePlayer.controller` を入れる。
3. `Tools > Skinned Anim FBX Exporter` で書き出す。

   | 項目 | 値 |
   |---|---|
   | FPS | 30 |
   | 水平方向のルートモーションを除去 | ON、対象ボーン `root,pelvis` |
   | 上下(Y)の移動も除去 | **OFF**(初期値は ON なので外す) |

4. `DxLibModelViewer_64bit.exe` のコピーを作業フォルダに置き、`Setting.ini` を `LoadAndSave=1`・**`SaveAnimKey16bit=0`** にして FBX を渡す。
   `SaveAnimKey16bit=1`(初期値)だと、FBX から読んだアニメのキーを16ビットに縮めるときに壊れることがある(11/24、97本のうち `Jump_Combat_Start_F_0` / `Jump_Combat_Loop_0` / `Hit_Combat_F` の `calf_l` の位置が 10³¹ になり、左脚が画面の外まで伸びた。FBX は正常だった)。
   手順5の `animmerge` は今まで通り16ビットで保存してよい(縮めずに変換した mv1 からなら壊れなかった)。
   同じフォルダに mv1 ができる。ビューアは自動では閉じないので、mv1 ができたら閉じる。96本で約4分かかる。
5. `build.bat` でビルドして、メッシュとアニメを合わせて保存する。

   ```
   animmerge.exe 今のPlayer03.mv1 手順4のmv1 Player03_new.mv1 ログ.txt
   ```

6. `Player03_new.mv1` を `2026_winter/data/Player03.mv1` に上書きして、`data/Player_CSV/Player01Anim.csv` に key を足す。

## 作ったアニメ(Unity の `Assets/Editor/PlayerAnimTool.cs`)

| mv1 のアニメ名 | アニメCSVの key | 中身 |
|---|---|---|
| `Boss_LeapSlash` | `BossLeapSlash` | Kingmech のトドメ。全74F(30fps)。地上の暗殺(`Execution_02`)の0〜5F(しゃがんで踏み切り) → 空中から床への斬り2(`Attack_Air_To_Floor_Start_02`)で12Fまでに振りかぶり、12〜22Fは頂点で振りかぶったまま、22〜26Fで振り下ろし → `Loop_02`(刃を下に急降下)26〜30F → 空中から床への斬り1の着地(`End_01`、刃を後ろへ振り抜いてしゃがむ)30F〜、35Fで着地、36〜50Fしゃがみ、そのあと立つ。**空中は体の高さを立った高さにそろえてある**(跳ぶ高さはゲームの移動で付ける) |

| `Kick_Up` | `KickUp` | 蹴り上げ(サマーソルト。NG4 の録画 `ref_kickup/` をまねた)。全32F。`Ninja_Acrobatic_Backflip` のしゃがみ〜宙返り(2→26F を 0→22F に詰める)に、右脚を真上へ振り上げる筋肉の値を足す(`Right Upper Leg Front-Back` は**負が前**。-1.3 で 9F に足が腰の 0.84m 上=当たる所)。右腕・指は切り上げ(`Attack_Up_Floor_To_Air_02`)の空中の姿勢で固定(宙返りは素手のため)。**着地しない**: 18〜25F で切り上げの最後の空中の構え(膝を畳んで刀を立てる)へ混ぜて終わる。空中は立った高さ |
| `Spin_RiseSlash` | `SpinRiseSlash` | X→X→Y の回転斬り上げ(`ref/README.md`)。全34F。0〜4F `Attack_Up_01` の低い構えへ沈む → 4〜7F 刃を下げる → 7〜18F 体を横に2回転(だんだん遅く)しながら `Attack_Up_Floor_To_Air_02` の斬り上げを進める → 18F〜 振り抜いたまま止まる。回転は体(RootQ)に掛けるので書き出し後も残る。回る向きは .cs の `kSpinDir` |

- 作る: `PlayerAnimTool.MakeAndExportKickSpin`(上の2本を作って `Library/PlayerAnimExport/KickSpinOnly.fbx` に)。`ExportAll` は3本とも入れる
- 動かして見る: `animplay.exe Player03.mv1 アニメのmv1 刀のmv1 [X Y]`(左のモニターにウィンドウ。横・前・斜め上でループ再生、←→で切り替え・Space で止める・`,` `.` でコマ送り・↑↓で速さ。ビルドは `build.bat` の cl の行のファイル名を変えて)。確認用の画像は `preview/`
- 作る: `PlayerAnimTool.MakeAndExportLeap`(クリップ `Assets/PlayerAnim/Boss_LeapSlash.anim` を作り、それだけを `Library/PlayerAnimExport/LeapOnly.fbx` に書き出す。確認用)。つなぐフレームは .cs 先頭の `kExecToStart0` など
- 全部書き出す: `PlayerAnimTool.ExportAll`(`TruePlayer.controller` の96本＋作ったクリップを `Library/PlayerAnimExport/PlayerAll.fbx` に)。そのあとは上の手順4〜6。ビューアの変換の後に `../EnemyAnim/animcmp.exe ログ 前のmv1 新しいmv1` で今のアニメが変わっていないか見る
- バッチ: `Unity.exe -batchmode -projectPath ninja_motion -executeMethod PlayerAnimTool.XXX -quit`(結果は `Library/PlayerAnimTool.result`)。Unity のエディタが開いていると動かない
- 刀を持たせて見る(DxLib、ウィンドウは出さない。`build.bat swordshot` / `build.bat swordall`):
  - `swordshot.exe 出力.png Player03.mv1 元のアニメ名 新しいアニメのmv1 刀のmv1 枚数`: 元のアニメ(横)と新しいアニメ(横・前)を並べる。新しいアニメは別の mv1 の0番を `MV1AttachAnim` で借りる
  - `swordall.exe 出力.png Player03.mv1 アニメのmv1 刀のmv1 枚数 [0=横 1=前] [始まりのフレーム 終わりのフレーム]`: その mv1 の全部のアニメを1行ずつ(材料を選ぶとき。`PlayerAnimTool.ExportCandidates` で候補を書き出す)
  - 刀の付け方は `PlayerWeapon::CalcWeaponMatrix` と同じ(`hand_r`、回転 X90・Y90+90・Z0-90、0.6倍)。長い刀は `data/Player_Weapon/red_katana_ult.mv1`

## 確認すること

- ログの `frame differs` が0、`tris` が前後で同じ、`anims` が足した分だけ増えている。
- 今までのアニメが変わっていない(骨の位置を比べる)。上の設定なら、95本のうち94本は差が 0.1 未満、`Run_Combat_to_Run` だけ最大 3.02 だった。
  - `../EnemyAnim/animcmp.exe ログ 前のmv1 新しいmv1` で `maxDiff=inf` や大きな値が出たら、手順4の `SaveAnimKey16bit` を確かめる。
  - 11/24 に `SaveAnimKey16bit=0` で作り直したときは、96本の差が最大 0.17(指先)、長さはすべて同じだった。
- .x を通してアニメを差し替えるのは不可(キーが行列になり、1フレーム短くなって骨が縮む)。

## 注意

- 骨の数・並びが同じモデルからしか持ってこられない。`animmerge` は違うと保存せずに終わる。
- `SK_VEX_01` を使う(三角形 95049・Transform 190 が `Player03` の元)。`SK_VEX_03` は別のモデル。
