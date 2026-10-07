# 2026_winter ゲーム本体 作業メモ

プロジェクト全体のサマリはリポジトリ直下の `CLAUDE.md`。ここには実装の詳細・作業ログ・TODO を書く。

---

## 作業ログ

### 2026-10-07 血殺(必殺技)演出・血しぶきエフェクト

#### 1. 必殺技中の赤いフォグ(お試し版)
- `Stage/BloodKillFog.h/.cpp` を新規作成。必殺技中に背景を赤で塗り、ステージだけに赤いフォグ＋暗い赤のカラースケールをかける。
- 発動条件は `BattleManager::GetIsUltimating()`。`SceneMain::NormalUpdate` でかかり具合(m_rate)を進め、`NormalDraw` で
  背景塗り → (キャラ描画) → ステージ前後に BeginStage/EndStage。キャラ・エフェクトにはフォグをかけない。
- 開始8F/終了15Fで実フレームのフェード(タイムスケールの影響なし)。
- **まとめてオフ**: `BloodKillFog.h` の `#define BLOOD_KILL_FOG_ENABLE 1` を 0 にすると全関数が何もしなくなる。
  完全に消すときは `[BloodKillFog]` で検索して呼び出しを消す(SceneMain.h/.cpp、Stage.h の GetStageViewHandle、vcxproj)。
- 調整値は `BloodKillFog.cpp` 先頭(フォグ距離 300/2000、背景＝フォグ色 120,0,0、ステージのカラースケール)。

#### 2. 血しぶきエフェクト(Effekseer MCPで制作)
- 場所: `data/Effect/Blood/`(テクスチャは `Blood/Texture/`)。1/20スケールで作成し、ゲームでは20倍で再生。
- 参考: NINJA GAIDEN 4 のプレイ動画(画面録画 2026-10-07 135639 / 225151)。
  原作の血は「つやのない平らな筋」＋「つやのある細い液体の糸」で、全方向ではなく一方向に噴き出す。色は少し青みのある深紅。
- **色は直接入れている**(白ベース＋ゲーム側で色を掛ける方式はやめた)。
- パターン(どれも同じノード構成: Burst / Jet / DarkShred / Droplets / Stream(None)+StreamBody(Track))
  - `BloodSplash`(A): +Z(横)に噴き出す
  - `BloodSplash_B`(B): 真上(+Y)に噴き上がる
  - `BloodSplash_C`(C): 横に低く扇状に広がる、液体の糸が多め
  - `BloodSplash_B_White`: Bの白い版(原作の血殺画面の白い飛沫)。量を約2倍にし、+Z(奥)側に8割ほど飛ぶ。黒い筋(DarkShred)を混ぜている。
- 液体の糸は Ribbon ではなく Track で作る(TrackSizeFor=尾, Back=先頭。左右を暗く中央を明るくしてつやを出す)。
- `Blood/Texture/` に前の版の未使用テクスチャが残っている
  (BloodDrop, BloodBlob, BloodDot, BloodSplat, BloodSplat2, BloodSpike, BloodStream, BloodShred, BloodSheet, BloodMist)。
  使っているのは BloodShred2 / BloodFlake / BloodSplatFlat のみ。不要なら削除してよい。

#### 3. 血しぶきのゲームへの組み込み
- `System.h` の `AsyncData` に `BloodSplashEffectA/B/C/White` を追加、`System.cpp` で読み込み・解放。
- `EnemyBase::OnDamage()` で再生(HPを減らした直後)。
  - 通常時: A/B/C からランダム(`GetRand(2)`)
  - 必殺技中: 白い版
  - 位置: 敵の足元＋高さ100(やられ判定の高さ)、スケール20、+Z を「プレイヤー→敵」の向きに回転(血は敵の向こう側へ飛ぶ)
- **必殺技判定の注意**: 必殺技の攻撃が当たった瞬間は、`AttackCol` が `OnDamage` を呼んだ**後**に `SetUltStart` するので、
  `GetIsUltimating()` はまだ false。そのため `other.GetTag().role == Collider::ColRole::UltAttack` も見て必殺技中と判定している。
- 調整値は `EnemyBase.cpp` 先頭(kBloodSplashHeight / kBloodSplashScale / kBloodSplashUltSpeed)。

#### 4. エフェクトをタイムスケールに合わせる
- `SceneMain::NormalUpdate` の `UpdateEffekseer3D(GetTimeScale() / kEffekseerFps)`(kEffekseerFps=60)。
  引数は「1フレームで進む秒数」なので /60 が必要(/60 なしだと60倍速になる)。
- **全エフェクトがスローに従う**。スロー用に遅く作ったエフェクトがあれば二重に遅くなるので通常速度版に差し替えること。
- **EffectManager**(`Managers/EffectManager.h/.cpp`、CollisionManager と同じシングルトン)でエフェクトの再生と更新をまとめた。
  - `Play(AsyncData, pos, rotY, scale)` で再生してハンドルを返す。
  - `SetOwnSpeed(handle, speed)` で、そのエフェクトだけタイムスケールに関係なく speed の速さにする(Collider/Animation の ownTimeScale と同じ考え方)。
    再生が終わるまで `Update()` が毎フレーム `speed / タイムスケール` を設定し直す(タイムスケール0のときは割らない)。
  - `Update()` を `SceneMain::NormalUpdate` の最後で1回呼ぶ。中で `UpdateEffekseer3D(タイムスケール / 60)` もしている。
  - 持ち主(敵など)が消えてもエフェクトは再生が続くので、マネージャーで管理している。
- 必殺技中の白い血は `SetOwnSpeed(handle, kBloodSplashUltSpeed)`(0.65、EnemyBase.cpp 先頭)で、スロー中も一定の速さで飛ぶ。

---

## TODO / 次にやること
- [ ] ゲームを起動して見た目を確認(フォグの距離・ステージの暗さ、血の大きさ(20倍)、向き、白い血、スロー時の速さ)
- [ ] 多段ヒットで血が出すぎないか確認。多すぎるなら一定フレーム出さない制限を入れる
- [ ] 血殺演出の残り(2D極太斬撃、3Dの白い斬撃、「血殺」の文字、端を黒くするビネット、振動)
  - 描画順の案: 背景(赤) → ステージ(赤フォグ) → キャラ → 2D斬撃 → 3D白斬撃 → 血しぶき → ビネット → 文字
- [ ] フォグで色が濁るようなら、ステージ用のグラデーションマップシェーダーを検討
- [ ] ルートの CLAUDE.md の記載が `2026_summer/` のままなので、`2026_winter/` に直す
