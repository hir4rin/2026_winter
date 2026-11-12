# タイトルの斬撃の閃光(TitleSlash)

タイトル画面でロゴ「忍」を斬った瞬間に、斬り線に沿って走る一閃。2026-11-12。

## 条件
- スタイル: 中間(形は鋭い刃、光はにじむ) / 用途: ゲームのタイトル画面(2D) / カメラ: 正面のみ
- 要素: 斬り線に沿う一閃だけ。今の画面全体の白い光・斬り口の光の画像は残して重ねる
- 進め方: 一気に作って、ゲームの画面で確認

## 作り(TitleSlash_v1)
- 刃の長さ 10 単位(x=-5〜+5)。ゲームで斬り線の長さに合わせて拡大する
- ノード(描く順): Glow → Flash → Core → Head。全部 Add
  - Glow: TS_Glow、赤(255,36,48,210)。1F 遅れ、24F。左から伸びて太くなりながら消える
  - Flash: TS_Glow、白(255,220,214,150)。3F 遅れ、10F。線に沿った太い白い光が一瞬広がる
  - Core: TS_Core、白。16F。左端を止めたまま右へ伸び(位置と拡大の Easing、Middle あり)、細くなって消える
  - Head: TS_Head、白。7F。左から右へ走り抜ける光の頭
- テクスチャ: `gen_slash_tex.ps1`(白。端は alpha 0。色はノードで付ける)

## ゲームへの組み込み
- `data/Effect/Title/TitleSlash.efk`(+ Texture/)。`TitleScene::PlaySlashEffect` が斬り口の光が走り始めるフレームで出す
- `Effekseer_Set2DSetting(画面の幅, 高さ)` → `PlayEffekseer2DEffect`。位置は画面のピクセル、回転はラジアン(時計回り)、拡大 = 画面の刃の長さ(px) / 10
- 斬り線の中心と向きは `data/UI/Title/logo_cut.csv`、長さはロゴ画像で 450px(斬り線は約394px)
- `UpdateEffekseer2D()` は Update、`DrawEffekseer2D()` はロゴの後・画面の白い光の前

## 失敗・注意
- 撮影でゲームを閉じそこねると、次の起動が DxLib の二重起動チェックで落ちる(Log.txt に「二重起動されています」)
- 起動からタイトルまでの時間がぶれるので、斬る瞬間を撮るときは間隔の短い連写で広く撮る
