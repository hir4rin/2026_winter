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

### 2026-10-08 UIManager の導入
- `UI/` に `FontManager` / `UIBase` / `UIManager` を追加し、vcxproj に登録した。
- UIManager は System 共通にはせず、**シーンごとに持たせる**方針(UIの寿命をシーンに合わせる・ポーズ中に下のシーンのUIを止めやすいため)。
  - `SceneMain` が `std::unique_ptr<UIManager> m_uiManager` を持つ。中の各UIは `shared_ptr<UIBase>`(シーン側でも参照を持てる)。
- 初期化は場面ごとに分ける: `UIManager::TitleInit()` / `InGameInit()`。
  中でUIを生成して `Add` し、`InitAll()` で各UIの `Init` を呼んで画像などのハンドルを生成する。前の場面のUIは `Clear()` で外す。
  ハンドルの解放は各UIのデストラクタで行う。
- SceneMain: `Init()` の最後で `InGameInit()`、`NormalUpdate` のカメラ更新後に `Update`、`NormalDraw` の3D/エフェクト描画後に `Draw`。
- UIBase の座標は `Vector3 m_pos`(floatなので描画時に int へキャストする)。
- **血殺UI**: `UI/KessatsuUI.h/.cpp`(UIBase継承)。2026_summer_refactoring の `GameScene::DrawUltKessatsuUI / DrawLastHitKessatsuUI` を移植。
  - 画像は `data/UI/blood_UI.png`(血) / `satu_UI.png`(殺) / `splash2_UI.png`(液体) をコピー。
  - `BattleManager::GetIsUltimating()` の間だけ表示。カメラが FinishingFirst/SecondCamera のときはラストヒット用の配置。
  - 元はレンダーターゲットに描いて引き伸ばしていたが、`Game::ScaleX/ScaleY/GetScale` で直接描画するようにした。
- **UIの描画レイヤー**: `UIBase::GetLayer()`(`UILayer::Back` / `Front`、基本は Front)。
  - `UIManager::DrawBack()` は Back だけ、`Draw()` は Front だけを描く。
  - SceneMain::NormalDraw の順番: 背景 → 敵 → ステージ → **DrawBack** → プレイヤー → エフェクト → **Draw**。
    2DのUIは深度を書かないので、後から描く3Dに上書きされる。そのため元の「プレイヤー → 敵 → ステージ」を並べ替えて、プレイヤーを最後にした。
  - KessatsuUI は Back(元と同じく文字がプレイヤーの後ろに出る)。

### 2026-10-08 頂点タイプ別シェーダー・血殺中に敵を黒くする
- `Managers/ModelShaderManager.h/.cpp`(シングルトン)を追加。SampleTPSGame の「モデルごとに4ボーン/8ボーンを分ける」仕組みを移植。
  - `Shader/MV1VertexShader.hlsl` をマクロ(SKINMESH / BONE8 / BUMPMAP)を変えて6回コンパイルし、
    `DX_MV1_VERTEX_TYPE_〜` を添え字にした配列に入れる。描画時に `MV1GetTriangleListVertexType` で頂点タイプを調べて選ぶ。
  - 1つのモデルの中で頂点タイプが混ざっていたら、メッシュごとに `MV1DrawMesh` で頂点シェーダーを切り替えて描く。
  - FREE_FRAME(9ボーン以上)はシェーダーを用意していないので、DxLibの標準シェーダーで描く(黒くならない)。
  - シェーダーは**実行時に** `D3DCompileFromFile` でコンパイルする(vcxprojでは None 扱い。ビルドでコンパイルしない)。
    `Shader/` の .h(VertexShader.h など)は 2026_summer からコピーしたDxLibのシェーダー用ヘッダ。
- 血殺中に敵を黒くする(2026_summer の EnemyBlackPS を移植): `Shader/EnemyBlackPS.hlsl`。
  - テクスチャのアルファだけ使い、色は黒(`kBlackColor`)。透明部分は clip で抜く。
  - PSの入力は VS_OUTPUT の先頭3つ(COLOR0/COLOR1/TEXCOORD0)だけ。BUMPMAP のときだけ VS_OUTPUT の途中に接線が入るので、全頂点タイプで共通な先頭だけ受け取る。
  - `SceneMain::NormalDraw` で `m_enemyManager->Draw()` の前後に `SetBlackMode(GetIsUltimating())` / `SetBlackMode(false)`。
  - 敵側は `MV1DrawModel` を `ModelShaderManager::GetInstance().DrawModel()` に置き換え(EnemySwordman本体・EnemyPart)。
    黒モードでないときはただの MV1DrawModel。
  - 2026_summer と同じく、黒くなるのは一瞬で切り替え(フェードなし)。

### 2026-10-08 プレイヤーの移動に慣性(加速・減速・切り返し)
- 速度をベクトルのまま「なりたい速度」へ1フレームに一定量ずつ近づける方式(速さだけでなく向きにも慣性がつく)。
- `m_rb.m_moveVel`(RigidBody) に今の移動速度を覚えておく。`m_rb.m_vel` は `Player::Update` で毎フレーム0にされ、
  CollisionManager でタイムスケールを掛けて書き換えられるので、慣性の保存には使えない。
- 共通処理は `PlayerState::UpdateMoveVel(targetVel, accel)`。近づけたあと `m_rb.m_vel = m_moveVel` を入れる。
  - Move: 入力の向き × `kMoveSpeed` へ `kAccel`(8Fで最高速)。FastRun: × `kRunSpeed` へ `kAccel`(15F)。Idle: 0 へ `kDecel`(8F)。
  - 向き(`m_targetVec`)は入力にすぐ合わせる。速度だけが遅れてついてくる。
- Idle/Move/FastRun の `Enter` で `m_rb.m_vel = m_moveVel` を入れる(切り替わったフレームに一瞬止まらないように)。
- `Player::ChangeState` で Idle/Move/FastRun 以外に移ったら `m_moveVel` を0にする(攻撃・回避の後に滑らないように)。`ForceIdleState` でも0にする。
- 真後ろへの切り返しは、一度止まってから逆に加速する(Moveで約16F)。重ければ、逆向き入力のときだけブレーキを強くする。

---

## TODO / 次にやること
- [ ] 実際のUIクラス(HPバーなど)を作って `InGameInit()` に登録する
- [ ] ゲームを起動して血殺中に敵(本体・取れた腕/頭)が黒くなるか確認
- [ ] ゲームを起動して見た目を確認(フォグの距離・ステージの暗さ、血の大きさ(20倍)、向き、白い血、スロー時の速さ)
- [ ] 多段ヒットで血が出すぎないか確認。多すぎるなら一定フレーム出さない制限を入れる
- [ ] 血殺演出の残り(2D極太斬撃、3Dの白い斬撃、「血殺」の文字、端を黒くするビネット、振動)
  - 描画順の案: 背景(赤) → ステージ(赤フォグ) → キャラ → 2D斬撃 → 3D白斬撃 → 血しぶき → ビネット → 文字
- [ ] フォグで色が濁るようなら、ステージ用のグラデーションマップシェーダーを検討
- [ ] ルートの CLAUDE.md の記載が `2026_summer/` のままなので、`2026_winter/` に直す
