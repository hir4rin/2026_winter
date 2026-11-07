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

4. `DxLibModelViewer_64bit.exe` のコピーを作業フォルダに置き、`Setting.ini` を `LoadAndSave=1` にして FBX を渡す。
   同じフォルダに mv1 ができる。ビューアは自動では閉じないので、mv1 ができたら閉じる。96本で約4分かかる。
5. `build.bat` でビルドして、メッシュとアニメを合わせて保存する。

   ```
   animmerge.exe 今のPlayer03.mv1 手順4のmv1 Player03_new.mv1 ログ.txt
   ```

6. `Player03_new.mv1` を `2026_winter/data/Player03.mv1` に上書きして、`data/Player_CSV/Player01Anim.csv` に key を足す。

## 確認すること

- ログの `frame differs` が0、`tris` が前後で同じ、`anims` が足した分だけ増えている。
- 今までのアニメが変わっていない(骨の位置を比べる)。上の設定なら、95本のうち94本は差が 0.1 未満、`Run_Combat_to_Run` だけ最大 3.02 だった。

## 注意

- 骨の数・並びが同じモデルからしか持ってこられない。`animmerge` は違うと保存せずに終わる。
- `SK_VEX_01` を使う(三角形 95049・Transform 190 が `Player03` の元)。`SK_VEX_03` は別のモデル。
