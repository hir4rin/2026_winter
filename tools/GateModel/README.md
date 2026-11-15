# GateModel（フェーズで開く扉のモデル）

ゲーム側は `2026_winter/Stage/Gate/Gate`、配置は `data/Stage/Gates/Stage%03d.csv`（番号は敵配置と同じ）、モデルは `data/Stage/Gates/` の下。
**扉の見た目はステージのアセットで作る**（自作した扉は雰囲気が合わなかった）。

## ステージ1: Unity の GateAssetTool.ExportCyberGates

`ninja_motion/Assets/Editor/GateAssetTool.cs`。Unity のエディタを閉じてから:

```
"C:\Program Files\Unity\Hub\Editor\2022.3.62f3\Editor\Unity.exe" -batchmode -projectPath C:\Users\tsyts\Desktop\programm\ninja_motion -executeMethod GateAssetTool.ExportCyberGates -quit -logFile gate_export.log
```

- 表 `CyberGates`（名前・区画・部品・幅・高さ・位置・向き・フェーズ）から、Cyberpunk Megapolis のプレハブを並べて扉を組み立て、`width` / `height` に合わせて伸ばす。
  - シャッターの壁: `CP_Wall_Roller_Shutters_01`。**開口の幅・高さに合わせ、壁の面から20cm奥（壁の厚みの中）に置く**（手前に立てると箱が飛び出して見える）。
  - 塀のゲート: `CP_Fence_B_Gate` ＋ 左右の `CP_Fence_B_2m_01` / `5m_01` ＋ 柱。何も無い広い道（ボス広場の後ろ）だけ。
  - 開口の幅・壁の面は `probe_gap.exe` で測る。
- 材質 `CP_Roller_Shutters` のサブメッシュを `Gate_〜_Shutter`（原点 = シャッターの上の真ん中）、残りを `Gate_〜_Static` に分ける。
- 区画の名前の子にして `CyberStageTool.WriteX` で .x に書く（区画の色・テクスチャの処理がステージと同じ）→ `convert.exe night` → `data/Stage/Gates/Cyber/`。使ったテクスチャを `CyberStage/tex` から `Cyber/tex` へ写す。
- **`data/Stage/Gates/Stage005.csv` も書き直す**（CSV を手で直しても、流し直すと消える）。
- `Survey`（候補のプレハブを撮って `Library/GateAssetShots/` に）、`SurveySubMeshes`（サブメッシュと材質）は調べる用。

## 列車: build_train_bulkhead.py（Blender）

```
copy  ninja_motion/Assets/TrainStage/Props/Textures の wagon_door.jpg / roof_rust.png / hazard.png → data/Stage/Gates/Train/tex/
blender -b --python build_train_bulkhead.py -- data/Stage/Gates/Train ninja_motion/Assets/TrainStage/Props/WagonDoor.fbx
blender -b data/Stage/Gates/Train/TrainBulkhead.blend --python ../FbxToMv1/export_x.py -- data/Stage/Gates/Train Bulkhead WagonDoor_L WagonDoor_R
convert.exe flat 各.x 各.mv1
copy_mat.exe data/Stage/TrainStage/TrainStage.mv1 19,10 data/Stage/Gates/Train/Bulkhead.mv1
```

- 通路車両の入口（幅12.4m・高さ5.7m）を赤さびの波板（車体の横と同じ `roof_rust.png`）の仕切りでふさぎ、真ん中に列車のアセットの貨車の扉 `WagonDoor` を左右に切って置く。開くと仕切りの裏へ左右に引き込む。
- `copy_mat.exe 元.mv1 番号[,番号...] 先.mv1 ...`: 色をゲームの `TrainStage.mv1` の材質と同じにする（番号を並べると先のマテリアル0,1,...に順に。19=波板、10=黄黒のテープ）。
- 部品の位置は `Train/parts.txt`。CSV `Stage004.csv` は手で書いている。
- Blender は `C:\Users\tsyts\tools\blender-5.3.0-windows-x64\blender.exe`（公式の zip 版）。
- 前の版（車両の元の扉を3本に切る `cut_train_door.py`）は、壁にしか見えなかったので消した。

## 場所を決めるツール（DxLib）

ビルドは `build.bat 名前`（例 `build.bat map_stage`）。

- `map_stage.exe 当たり判定.mv1 出力.png 中心x 中心z 幅(cm) [敵配置CSV] [エリアCSV]`
  真上から当たり判定を描く（壁=白、床は高さで青〜赤、10mの格子に座標）。敵（グループ＋フェーズ）とエリアも描く。
- `probe_gap.exe 当たり判定.mv1 x z 向き(x|z) 床を探す高さ [...]` → `probe_gap_log.txt`
  床の高さと、進む向きと直角の左右の壁の位置・幅・天井を測る。
- `extract_door.exe`: `GateBox.mv1`（当たり判定用の1m角の箱、面は外向き）を作る（前の版の扉の切り出しも出るが使っていない）。`check_box.exe` で面の向きを調べる。
- `preview_gate.exe`: 前の版の開き方の確認用。今は使っていない。
