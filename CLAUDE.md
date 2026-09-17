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

## 運用ルール

- 実装作業の詳細（現在のタスク・TODO・設計方針・作業ログ）は
  `2026_summer/CLAUDE.md` に書く。ここはプロジェクト全体のサマリのみ。
- 作業を区切る際（PCを閉じる前など）に、次回すぐ再開できるよう
  `2026_summer/CLAUDE.md` の該当欄を更新する。
