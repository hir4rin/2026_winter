# 2026_summer リポジトリ概要

学校のPCと家のPCで会話履歴が自動で繋がらないため、作業の引き継ぎメモとして
このファイルを使う。Claude Codeは起動時にこのファイルを自動で読み込む。

## 構成

- `2026_summer/` — ゲーム本体（Visual Studio C++プロジェクト、`.slnx`/`.vcxproj`）。
  実装作業の詳細・作業ログ・TODOは `2026_summer/CLAUDE.md` を参照。
- `EffekseerForDXLib/` — Effekseer（エフェクト）ライブラリのDXLib連携用ファイル一式
- `testModel_Ninja/` — Blenderモデル（`player_color.blend`）とアニメーション素材
- `Xbox Series/` — コントローラーアイコン素材
- `進捗/` — 企画書（.pptx）、コスト表（.xlsx）、進捗メモなどのドキュメント類
- `tools/` — 作業用ツール（`BossArmExtract/`: ボスのmv1から腕を切り出すDxLibツール、`KingmechGatling/`: Kingmechのガトリングを切り出すDxLibツール、`TrainTunnel/`: ループ用トンネルを作るBlenderスクリプト、`TrainStage/`: 列車ステージを作るBlenderスクリプト(調整はUnityの `ninja_motion/Assets/Editor/TrainStageTool.cs` で行う)、`FbxToMv1/`: Blender→.x→mv1 の変換とDxLibでのプレビュー、`CyberStage/`: ステージ1(Ch1)の設計と Unity での作り方。ツールは Unity の `ninja_motion/Assets/Editor/CyberStageTool.cs`、`MenuUIGen/`: メニューUIの仮素材PNGを作るPowerShellスクリプト、`AssasinUIGen/`: 暗殺UI・確殺UIの画像PNGを作るPowerShellスクリプト、`HairLod/`: mv1の髪のメッシュをBlenderなしで間引くDxLibツール、`PlayerAnim/`: メッシュはそのままで Player03.mv1 にアニメを足す手順とツール、`BloodDecalGen/`: 床に残る血の跡・画面に付く血(`MakeScreenBlood.ps1`)などの画像PNGを作るPowerShellスクリプト、`ControlGuideGen/`: 操作説明の画像(`進捗/操作説明_pad.png`)を作るPowerShellスクリプト、`TitleLogoGen/`: タイトルロゴ(「忍」、ゲーム用は `data/UI/Title/logo.png`)を Illustrator で作るスクリプト(PowerShell から COM で `.jsx` を実行)、`TitleSlashFx/`: タイトルでロゴを斬った瞬間の閃光(Effekseer)の元データとテクスチャ生成スクリプト、`RankEmblemGen/`: リザルトのランクの紋章(C/B/A/S/SSS)の PNG を作るスクリプト(Edge のヘッドレスで canvas を描いて保存)、`GateModel/`: フェーズで開く扉のモデルの作り方(ステージ1は Unity の `Assets/Editor/GateAssetTool.cs` でアセットから書き出す、列車は車両の元の扉を Blender で切る)と、扉の場所を決める DxLib ツール(真上からの地図・幅の測定)）
- `tools/ResultIconGen/`: リザルトの項目アイコン(技・殺・時・道・死、筆の円相＋垂れ)を Illustrator で作るスクリプト(`MakeResultIcon.ps1`。出力は `data/UI/Result/icon_tech/kill/time/item/death.png`。書体は KSO黒龍爽、筆の型は jsx の `GAME_STYLE`(F)。`out/_brushes.png` に型の比べ。jsx・ps1 は BOM 付きで保存)
- `tools/TransitionMock/`: ポーズ・リザルトへのトランジション案の動くモックアップ(`transition_mock_ng4.html` をブラウザで開く。NG4 の UI 記事の手法で12案。検討の記録は `2026_winter/CLAUDE.md` の 11/20)
- `tools/EnemyAnim/`: 敵(Enemy.mv1 / EnemyGun.mv1)にアニメを足す手順とツール(Unity の `ninja_motion/Assets/Editor/EnemyAnimTool.cs` でクリップを作って書き出し → ビューアで mv1 → `animmerge` でメッシュはそのままアニメだけ入れ替え。比べる `animcmp`・撮る `shot`)
- `tools/SwordSlashProbe/`: 攻撃のアニメで刀の先の動きを測るDxLibツール(`probe.exe プレイヤー.mv1 刀.mv1 ComboChain01.csv ログ`。刀の軌跡を出す区間の目安)
- `EffectTools/SwordTrail/`: 刀の軌跡(風切り)の調査・制作の記録と画像の素材のスクリプト
- `EffectTools/StartLand/`: 登場演出の着地エフェクト(`data/Effect/StartMovie/StartLand.efk`)の地面のひびの画像を作るスクリプト(`gen_crack.ps1`)とゲームで撮った確認の画像
- `tools/FrameCheck/`: 画面に実際に出た更新を測るツール(`dupmon.exe`。同じ絵が2回出た=カクついた回数が分かる。ゲームの中の記録は CPU 側の時間なので見えない)
- `.claude/skills/sakurai/`: 「桜井政博のゲーム作るには」の原則をまとめたスキル(kociden/sakurai-skill から取り込み)。「桜井さんならどうする」「手応えをレビューして」などで読み込まれる
- ステージ・敵配置の Unity(`ninja_motion`、家PCは `Desktop\programm\ninja_motion`)は別の非公開リポジトリ `hir4rin/WinterGame2026_ForStageAndModel`。買ったアセットは入れていない(このリポジトリは公開なので入れない)。別のPCでの開き方・アセットの一覧はそのリポジトリの `README_アセット.md`
- Blender は `C:\Users\tsyts\tools\blender-5.3.0-windows-x64\blender.exe`(公式の zip 版。Steam の Blender は exe が無くなっていた)

## 運用ルール

- 実装作業の詳細（現在のタスク・TODO・設計方針・作業ログ）は
  `2026_summer/CLAUDE.md` に書く。ここはプロジェクト全体のサマリのみ。
- 作業を区切る際（PCを閉じる前など）に、次回すぐ再開できるよう
  `2026_summer/CLAUDE.md` の該当欄を更新する。
