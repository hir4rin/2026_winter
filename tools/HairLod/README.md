# HairLod

mv1 の中の髪のメッシュを、Blender や元の FBX を使わずに減らすツール。
髪が「格子状の細長い板の集まり」のときだけ使える(`Player03.mv1` の髪は 横4 × 縦16〜28マスの板が207枚)。
板ごとに格子の行・列を間引くので、残った頂点の位置・法線・UV・ウェイトは元のまま。

## 手順

`build.bat` でビルドする。作業フォルダに元の mv1 とテクスチャ(`Player01.fbm`)を置いて、そこで順に実行する。
元の mv1 は間引く前のものを使う(今の `data/Player03.mv1` は間引き済み。元は git の履歴にある)。

```
xdump.exe Player03.mv1 mesh.x
hairlod.exe mesh.x lod.x SK_CSF_F_HAIR_01 2 2 lod_log.txt
powershell -File xdedup.ps1 -In lod.x -Out lod_u.x
hairsave.exe Player03.mv1 lod_u.x Player03_new.mv1 save_log.txt
hairanim.exe Player03.mv1 Player03_new.mv1 anim_log.txt w
```

| ツール | すること |
|---|---|
| `xdump` | mv1 のメッシュを .x に書き出す(`MV1SaveModelToXFile`) |
| `hairlod` | .x の指定フレームのメッシュの行・列を間引く。引数は 入力 出力 フレーム名 長さの間引き 幅の間引き ログ |
| `xdedup.ps1` | 同じ名前の Material を1つにまとめる |
| `hairsave` | .x を読み、マテリアルの色を元の mv1 に合わせ、アニメは元の mv1 から持ってきて mv1 に保存する |
| `hairanim` | 2つの mv1 を全部のアニメで撮って比べ、差の大きいポーズを PNG に出す |

## 注意

- DxLib が書き出した .x はメッシュごとに同じ名前の Material があり、そのまま `MV1LoadModel` するとメモリが壊れて落ちる。`xdedup.ps1` は省かない。
- まとめるので、マテリアルの数は減る(`Player03` は 23 → 6)。マテリアルを番号で決め打ちしているコードがあると影響する。
- .x を通すので、頂点の丸めで全身がごくわずかに変わる。
- 格子にならない板(同じ板が重なっているものなど)は、そのまま残す。`lod_log.txt` の `irregular` が数。
