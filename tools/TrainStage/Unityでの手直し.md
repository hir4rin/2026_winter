# 列車ステージを Unity で手直しする

Unity プロジェクト: `C:\Users\tsyts\Desktop\programm\ninja_motion`(Unity 2022.3.62f3)
ツール: `Assets/Editor/TrainStageTool.cs`(メニュー `Tools > Train Stage`)

## 1. 開く

`Assets/Scenes/TrainStageScene.unity` を開く。Hierarchy はこうなっている。

| オブジェクト | 中身 | ゲームへ |
|---|---|---|
| `TrainStage` | 見た目。`Locomotive` / `Car1_Flat` … `Car6_Flat` / `Ramp_Up` / `Ramp_Down` ごとに分かれている。中に `Mesh`(車両本体)と `ContainerXX`(コンテナ1個ずつ) | TrainStage.mv1 |
| `TrainStage_Collision` | 当たり判定。同じ車両名の下に箱が1個ずつ(半透明の緑) | TrainStageCol.mv1 |
| `EnemySpawns` | 敵の配置(`EnemySpawnPoint`) | Enemies004.csv |
| `TrainTunnel_Reference` | トンネルの目安(書き出さない) | - |

- Unity の 1m = ゲームの 100。軸はそのまま。プレイヤーは **+Z** へ進む(機関車が -Z、最後尾が +Z)
- 1両目の原点の床(高さ0)は動かさない(ゲームは原点の床を高さ0に合わせて置く)

## 2. 見た目を直す

- **動かす・回す・大きさを変える**: そのまま Transform を変える(車両ごと動かすなら `Car1_Flat` などを選ぶ)
- **増やす**: Ctrl+D で複製
- **消す**: 削除する。消したくないけど試しに外すだけなら、チェックを外して非アクティブにする(書き出されない)
- **素材を置く**: `Assets/TrainStage/Props/Prefabs` のプレハブを、`TrainStage` の中の車両(例 `Car6_Flat`)へドラッグ

  | プレハブ | 大きさ(幅×長さ×高さ m) |
  |---|---|
  | Container_Orange / Container_Rust | 4.5 × 8 × 4(扉が +Z 側) |
  | CorridorCar / CorridorDoors | 15.9 × 40.5 × 6.3(通路の車両と、両端を閉じる扉) |
  | Locomotive | 10.4 × 75.2 × 13.8 |
  | WagonDoor / ScanDoor | 貨車の扉(高さ3.8) / ドア(高さ2.6) |
  | SciCrateA / SciCrateB | SF木箱 |
  | HazardTape | 黄黒のテープ 0.35 × 1(長さは Z の大きさで伸ばす) |
  | RustPanel | 赤いさびの波板 1 × 1(壁や屋根に貼る) |
  | TunnelSection | トンネル1区間(目安用) |

- **色・テクスチャ**: `Assets/TrainStage/Materials`(ステージ)・`Assets/TrainStage/Props/Materials`(素材)のマテリアルの Albedo の色とテクスチャを変える。
  色が暗いほどゲームでも暗い(ゲームでは「拡散=色×0.1、環境=色×0.9」にして描く)。光らせたい物は Emission をオンにする

## 3. 当たり判定を直す

- 見た目を動かしたら、**`TrainStage_Collision` の中の同じ車両の箱も動かす**(見た目と当たり判定は別々)
- 新しく置いた物に当たり判定を付ける: その物を選んで `Tools > Train Stage > Add Collision Box For Selection`
  → 外形に合わせた箱が `TrainStage_Collision` の同じ車両の下にできる(回っている物は外形の箱になるので、必要なら回して合わせる)
- 自分で足すなら、`TrainStage_Collision` の下に Cube を作って大きさを合わせるだけでもよい(中身のメッシュならどんな形でも書き出される)
- 床は上を向いている面が床になる(箱ならそのままでよい)。スロープは傾けた箱でもよい
- 列車の左右の縁と前後の端の高い箱(`Outer`)は、列車の外へ出られないようにする見えない壁。床を広げたらこれも広げる

## 4. 敵を直す

- `EnemySpawns` の中の敵を動かす・回す(青い線の向きが正面)。Inspector で種類(type)・フェーズ・初期状態(Idle / Patrol / Watch)・グループを変える
- 増やすときは Ctrl+D。巡回させるときは敵を選んで `Tools > Enemy Spawn > Add Waypoint`
- フェーズの進み方は `data/Stage/PhaseFlow/PhaseStage004.csv`(今は A → B → C → D → GameClear。グループ名で全滅を待つ)
- ゲームのCSVを直接直したときは `Tools > Train Stage > Reload Enemies From Game` で読み直す

## 5. ゲームへ書き出す

`Tools > Train Stage > 2. Export To Game (mv1)` を押すだけ。
ゲームの `data/Stage/TrainStage/TrainStage.mv1`・`TrainStageCol.mv1`・`data/Stage/Enemies/Enemies004.csv` が上書きされる。
ゲームはビルドし直さなくてよい(起動し直してステージセレクトの「列車ステージ」)。Console に結果が出る。

## 注意

- `Tools > Train Stage > 1. Import From Game Data` は **シーンを作り直す**(Unity での調整が消える)。普段は使わない
- `tools/TrainStage/build_all.sh`(Blender で作り直す)を実行すると、ゲームの mv1 が Blender 版で上書きされる。Unity で調整した後は使わない
- Git で管理されていない(Unity プロジェクトはリポジトリの外)。大きく変える前はシーンを複製しておくと安心
