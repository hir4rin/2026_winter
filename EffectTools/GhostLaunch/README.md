# GhostLaunch(スキル0の白い分身)

- 書き出し: `2026_winter/data/Effect/Skill/GhostLaunch.efk`(モデルは同じフォルダの `Model/`、テクスチャは `Texture/GhostNoise.png`)
- 版: `GhostLaunch_v1/`(採用)

## 作り
- 見た目はスキル1の分身(`data/Effect/Skill/GhostDash.efkefc` の Ghost ノード)と同じ設定。
- `Ghost01`〜`Ghost14` はそれぞれ1体だけ出す。出るフレームは 1 / 5 / 9 / 12 / 17、寿命22F。
  位置・回転・拡大は「生成時のみ」なので、ゲームで毎フレーム位置を動かすと、出た分身はその場に残る。
- 1/20 で作り、ゲームでは20倍で再生。回転はプレイヤーの `m_rotAngleY` そのまま(180度ずらさない)。

## モデルの作り直し
`tools/PlayerAnim/posemodel.exe 出力.efkmodel Player03.mv1 katana_blend.mv1 Attack_Up_01 コマ 0.05`
(コマは 1 / 4 / 7 / 10 / 14。スキル0はアニメを0.8倍で流すので、出るフレーム ≒ コマ ÷ 0.8)

## 注意
- MCP の `node_xml_set` で相対パスを入れると保存で落ちる。パスは `param_set` に絶対パスで渡す。
