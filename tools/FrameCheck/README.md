# FrameCheck（画面に実際に出た更新を測る）

ゲームの中の記録（`Log/FrameStats.txt` など）は CPU 側の時間なので、**モニターに同じ絵が2回出た（カクついて見えた）か**は分からない。
このツールは Windows の Desktop Duplication で、ゲームのウィンドウの範囲が画面上で書き換わった時刻を記録する。

```
build.bat
dupmon.exe 秒数 出力.txt [モニターの番号(0から)]
```

- `2026_winter.exe` のウィンドウを探し、そのウィンドウと重なる「変わった範囲」がある更新だけを数える（ほかのウィンドウの再描画は数えない）。
- 出力: 更新の間隔（ms）を1行ずつ。最後と標準出力に `frames / avg / max / gap>25ms / gap>40ms`。
  - 59Hz のモニターなら間隔はほぼ 16.7ms。**`gap>25ms` が「同じ絵が2回出た（1回飛んだ）」回数**。
- ゲームのウィンドウがあるモニターの番号を渡す（このPCはメインの DISPLAY1 が 0）。
- 例（ベンチと同時に動かす。PowerShell）:
  ```
  $p = Start-Process .\x64\Release\2026_winter.exe -ArgumentList "-bench=3000 -stage=3" -PassThru
  Start-Sleep -Seconds 8
  ..\tools\FrameCheck\dupmon.exe 35 dup.txt 0
  ```
