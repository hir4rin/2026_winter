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

### 2026-10-10 被弾側で弱攻撃・強攻撃を判別できるようにした
- `ComboChain01.csv` の index の後(攻撃力の前)に `attackType` 列を追加。値は `AttackType`(-1:なし 0:弱 1:強 2:スキル)。
  - 切り上げ攻撃(7)・ダッシュ攻撃(14)はひとまず -1。
- `ComboNodeType` に `AttackTypeCol = 4` を追加し、後ろの列番号を1つずつずらした(`AttackType` だと enum class と名前がぶつかる)。
- `ComboNode::attackType` → `CreateAttackCol` で `AttackData::attackType` に入れる。敵の `OnDamage` では `data.attackType` で判別できる。
  - `AttackData::attackType` は初期値 None なので、ドロップ攻撃・必殺技・敵の攻撃は None。
- `m_attackType`(PlayerStateAttack)はコンボ2段目以降 None になるので、判別には使わないこと。

### 2026-10-11 敵の攻撃予備動作ステート(EnemyAttackPre)
- `Character/Enemy/State/Attack/EnemyAttackPre.h/.cpp` を追加。遷移は **Chase → AttackPre → Attack**
  (予備動作は攻撃直前・攻撃距離で出す方針。先に予備動作→追跡だと、攻撃までの時間が距離で変わって読めなくなるため)。
- アニメは CSV の `AttackPre1`(Combo_Attack_Wave_05_02)。ループなし、終わったら `EnemyAttack` へ。
- 予備動作中はその場で止まる。アニメの rate が `kAttackPreTrackEndRate`(0.6)まではプレイヤーの方を向き、`m_targetPos` も更新する
  (EnemyAttack は m_targetPos に向かって踏み込むため)。それ以降は向きを固定して、見てから避けられるようにする。
- 呼び出し元を差し替え: `EnemyChase`(2か所)、`EnemyBase::NextAfterIdle`。`SetDebugIdle` の対象にも追加。
- 攻撃権の処理は未実装。`Enter` / `Exit` に TODO を置いてある(中断されたときに攻撃権を返すのを忘れないこと)。

### 2026-10-11 敵の戦闘まわりのバグ修正(4件)
- **攻撃のクールタイム**: `m_attackCoolTime` を減らす処理が無く、1回攻撃した後は `CanMeleeAttack` がずっと false だった。
  `EnemySwordman::Update` で毎フレーム減らすようにした。
  - `EnemyChase` の「目的地に着いたら距離に関係なく攻撃」(このバグの回避策だった)をやめ、クールタイム中なら `EnemyCaution` へ。
- **死亡処理**: HPを減らすだけで死ななかった。`EnemyBase::OnDamage` に HP<=0 の処理を追加。
  - 地上で普通の攻撃 → その場で `EnemyDie`
  - 叩きつけ(knockBack.y<0)・キリモミ → `m_isDieOut` を立てて HitDrop へ。着地で `EnemyDie`(元からある処理)
  - 空中・打ち上げ → HP1を残す(空中では死なない)
  - 死亡済み・死亡吹っ飛び中の被弾チェックをガード判定より前に移動(死体がガードに入らないように)
  - `EnemyDie::Enter` でやられ判定を消す。`EnemyBase::ApplyPos` で死体は AirFall にしない(着地後Idleで生き返るのを防ぐ)
- **攻撃判定の位置**: AttackCol::ApplyPos の前方50＋半径80で、背中側にも30はみ出していた。
  `EnemyAttack` で前方に60足して(中心は110前)、半径を70にした。後ろの端が体の半径(40)より前になる。
  `EnemyAttack::Enter` の `ToPlayerLook()` を削除(予備動作で固定した向きを向き直さない)。
- **アニメの二重更新**: `EnemySwordman::Update` と各State(Idle/Guard/GuardBreak/Hit系/Air系/Die/KnockBack/KnockDown)の両方で
  `m_anim.Update` していて、それらのStateだけ2倍速だった。Stateからは消し、Swordmanで `m_anim.Update(m_ownTimeScale)` を1回だけ呼ぶ。
  - **アニメ速度・終了タイミングがこれまでの半分の速さに戻る**ので、Idle/被弾/ガードブレイクなどの見た目や長さの再調整が要るかも。
  - 今後Stateを作るときも、State内で `m_anim.Update` を呼ばないこと。

### 2026-10-11 OnDamage 中の接地判定
- CollisionManager は判定の前に `m_isFloor` を false に戻し、床への吸着(SnapToGround)は全部の OnCollision が終わってから行う。
  そのため `OnCollision` / `OnDamage` の中では `IsFloor()` がほぼ false になる。
- `Collider::WasFloor()`(前のフレームの接地状態)を追加。`EnemyBase::OnDamage` のガードの条件(弱攻撃＋地上)と、地上で死ぬ判定はこちらを使う。
- 当たり判定の処理の中で接地を見るときは `WasFloor()` を使うこと。

### 2026-10-11 敵のデバッグ表示
- 敵の頭の上に「ステート名＋情報」「HP / 攻撃クールタイム(CT)」を出す。足元に向きの線(先に球)、予備動作・攻撃中は踏み込み先(m_targetPos)への線。
  - 文字色はステートの種類: 移動・待機=白、予備動作=オレンジ、攻撃=赤、被弾・空中=黄、ガード=水色、死亡・処刑=灰。
  - 情報を出しているステート: AttackPre(経過/長さ、向き固定後は「固定」)、Attack(rate、判定中は「判定ON」)、Idle・Caution(経過/長さ)、Guard(ゲージ・経過)。
- **F3** で表示を切り替える(最初は表示)。`SceneMain::NormalDraw` の `#ifdef _DEBUG` 内、UIの後で `EnemyManager::DebugDraw()` を呼ぶ。
- 仕組み: `EnemyStateBase` に `GetName()`(純粋仮想)/ `GetDebugCategory()` / `GetDebugInfo()` を追加。
  **新しいStateを作るときは GetName を必ず実装する**(種類がMove以外なら GetDebugCategory も)。
  各Stateの `DebugDraw()` は3Dの補助線用として残した(Patrolの経路・視界)。左上に出していた "EnemyState:〜" の文字は削除。
  `EnemyBase::DebugDraw()` が共通処理(純粋仮想をやめて実装を持たせ、EnemySwordman の override は削除)。
- 当たり判定の線の色を役割で分けた(`Collider::DebugDraw`): 敵の攻撃=赤、プレイヤーの攻撃=薄い赤、やられ判定=青。
- `std::format` の書式文字列に日本語を入れないこと(Shift-JISの2バイト目が `{` `}` と同じ値になることがある)。日本語は引数で渡す。

### 2026-10-11 敵の判断のステート遷移を仮想関数にした
- **詳細は `Character/Enemy/EnemyStateTransition.md`**(どの関数がいつ・どこから呼ばれるか、初期の遷移先、この先の使い方)。
- `EnemyBase` に `NextOnArrived` / `NextAfterCaution` / `NextAfterAttack` / `NextOnPlayerFound` / `NextAfterRecover` を追加(既存の `NextAfterIdle` と合わせて6個)。
  各Stateの判断の遷移(計11か所)をこれらの呼び出しに差し替えた。初期の遷移先は元と同じなので、動きは変わらない。
- 打ち上げ→落下→着地、予備動作→攻撃などの「流れ」の遷移は各Stateに直接書いたまま。
- EnemySwordman ではまだ上書きしていない。

### 2026-10-12 攻撃の後隙ステート(EnemyAttackEnd)
- `Character/Enemy/State/Attack/EnemyAttackEnd.h/.cpp` を追加し、vcxproj/filters に登録。遷移は **Attack → AttackEnd → Back**。
  - `NextAfterAttack` の初期の行き先を AttackEnd に変更。`NextAfterAttackEnd`(仮想関数)を追加し、初期は Back。
- 中身: アニメは変えない(攻撃アニメはループなしなので最後の姿勢で止まる)。その場で止まり、向きも頭も動かさない。
  `kAttackEndTime`(40F、EnemyAttackEnd.cpp 先頭)で次へ。
- **後隙中は弱攻撃でもガードしない**(`EnemyBase::OnDamage` のガード条件に追加)。被弾すれば通常どおりHit系に移る。
- デバッグ表示の種類に `Recovery`(緑)を追加。`SetDebugIdle` の対象にも追加。
- 攻撃のクールタイム(120F)は今まで通り Attack の終わりで入るので、後隙の40Fもクールタイムに含まれる。

### 2026-10-12 弾きステート(EnemyParry)の土台
- `Character/Enemy/State/General/EnemyParry.h/.cpp` を追加し、vcxproj/filters に登録。**まだどこからも入らない**(土台だけ)。
- 中身: 仮のアニメ(BlockHit)、その場で止まってプレイヤーの方を向く、ガードゲージを0に戻す、`kParryTime`(30F・仮)で
  `NextAfterParry()`(仮想関数を追加、初期は AttackPre=反撃)へ。デバッグ表示はガード系の水色。
- 未定(TODOを置いてある):
  - 弾く条件(案: ガード中に弱攻撃でゲージがたまったら弾く、強攻撃ならガードブレイク。今はゲージがたまるとガードブレイク)
  - プレイヤーをひるませる処理(`Player::OnParried` を作り、ダメージ・無敵時間を通さずにひるませる。最初は PlayerStateHit を使い回す案)
  - 弾き用のアニメ(敵のアニメCSVに無い)
  - 弾いている間に攻撃が当たったときの扱い(案: 無視する)
- 調整の注意: プレイヤーのひるみ時間 > 反撃の予備動作 だと反撃が必ず当たる。ひるみ明けに回避が間に合う長さにする。

### 2026-10-12 弾かれたプレイヤーのステート(PlayerAttackParried)の土台
- `Character/Player/Defence/PlayerAttackParried.h/.cpp` を追加し、vcxproj/filters に登録。Player.h に include と friend を追加。
  (名前は依頼どおり。他のプレイヤーのStateは `PlayerState〜` なので、揃えるなら `PlayerStateAttackParried` に改名する)
- 中身: 仮のアニメ(Hit)、横の速度を止める、`kParriedTime`(30F・仮)たったら PlayerStateHit と同じく Idle/Move/Fall へ。
  ダメージ・無敵時間は無し。
- `Player::OnParried()` を追加(死亡中は何もしない)。`EnemyParry::Enter` から呼ぶようにした(EnemyParry自体はまだどこからも入らない)。
- 未定(TODO): 弾かれた用のアニメ、必殺技中・スキル攻撃中に弾かれたときの扱い、空中で弾かれたとき(今は30F浮いたまま止まる)。

### 2026-10-12 アニメのブレンドフレーム数を指定できるようにした
- `Animation::ChangeAnim` / `ChangeAnimWithModelHandle` の最後に `blendFrame` 引数を追加(デフォルト -1)。
  - 負の値: 今まで通り `kAnimChangeFrame`(20F)。0: 即切り替え(0除算しないよう AnimBlend で分岐)。
  - 例: `m_anim.ChangeAnim("Hit", false, 1.0f, -1.0f, 0.0f, 5.0f);`
- 違うモデルへの切り替え(Init経由)はブレンドしないので blendFrame は使われない。
- `ComboChain01.csv` の endFrame の隣に `blendFrame` 列を追加(空欄なら20F)。`ComboNodeType::BlendFrame = 21` を追加し後ろを1つずつずらした。
  `ComboNode::blendFrame` を通常攻撃・スキル攻撃・ダッシュ攻撃の ChangeAnimWithModelHandle に渡している。

### 2026-10-12 敵の攻撃データの外部化(読み込みまで)
- `data/Enemy/EnemySwordmanAttack.csv` を追加(Shift-JIS、ComboChain01.csvと同じく1行目列名・2行目説明・#はコメント)。今は「横斬り」1行だけで、値は今までのコードの定数そのまま。
- プレイヤー(ComboNode)と同じ形: DataManager は生データだけ持ち、解釈は EnemyBase でする。
- `EnemyBase.h` の先頭に、1行分の構造体 `EnemyAttackNode` と列番号 `EnemyAttackNodeType`(ComboNodeTypeと名前がぶつかるので enum class)。
- `DataManager::LoadEnemySwordmanAttackRawData()` / `GetEnemySwordmanAttackRawData()`(生データ)。
- `EnemyBase::InitializeAttackList(rawData)` で `m_attackList` を作る(EnemySwordmanのコンストラクタで呼ぶ)。要素の番号=index(CSVはindex順に並べる)。
  - 読み取りはプレイヤー(InitializeComboChain)と同じ書き方。列番号は `struct EnemyAttackNodeType { enum : int {...}; }`(ComboNodeTypeと名前がぶつからないように構造体に入れている)。
  - 空欄にできるのは「空欄なら〜」と書いた列だけ(再生速度・止めるフレーム・ブレンド・SE)。**最後の列(nextAttack)は空欄だと列数不足になるので -1 を入れる**。ステータスCSVも同じ書き方。
  `GetAttackNode(index)` / `GetCurrentAttackNode()`、`m_currentAttackIndex`。
- CSVの列: preBlendFrame・animEndFrame を追加、endTime → attackEndTimeFrame、attackColStart〜moveSpeedDuringCol を blendFrame の後ろへ。
- Qで `EnemySwordmanAttack.csv` を読み込み直す(EnemySwordman::Update。プレイヤーのComboChainと同時に読み直される)。
- AttackPre / Attack / AttackEnd はユーザーが node の値に置き換え済み。
- 攻撃の選択: `EnemyBase::SelectAttackIndex()`(クールタイム中なら-1、距離が attackRange 以内の攻撃からランダム。attackRange<=0 は単体では選ばない)。
  `NextAfterIdle` / `NextOnArrived` で選んで `m_currentAttackIndex` に入れる。Chase は「出せる攻撃があるか」だけ見る。`NextAfterParry` は0固定(仮)。
- Chase の近づく距離・Caution の回る半径は `GetMaxAttackRange()`。`CanMeleeAttack` と `kEnemyMeleeAttackRange`(3か所)は削除。
- 注意: `EnemyAttackPre` の `17.54f` は開始フレームではなく endFrame(引数の順番)。CSVの preAnimEndFrame に入れてある。

### 2026-10-12 敵のステータスの外部化(読み込みまで)
- `data/Enemy/EnemyStatus.csv`(Shift-JIS、1行=1種類、typeは配置CSVと同じ名前)。値は今までの定数そのまま。警戒移動は倍率ではなく速度(4×0.5=2)。
- 攻撃と同じ形: `DataManager::LoadEnemyStatusRawData()` / `GetEnemyStatusRawData()` / `ReloadEnemyStatusRawData()`(生データ)、
  `EnemyBase.h` に `EnemyStatus` / `EnemyStatusType`、`EnemyBase::InitializeStatus(rawData, type)` で自分のtypeの行を `m_status` に入れる。`GetStatus()`。
- EnemySwordman はコンストラクタと Q の読み込み直しで呼ぶ(type は `kStatusType` = "Swordman")。
- 各所の定数は `m_status.xxx` に置き換え済み(HP・当たり判定・部位破壊のボーン、速度、ガード、被部位破壊率、AIの判断、視界、ダウン時間)。
  HP・当たり判定は生成時にしか使わないので、Qで読み直しても今いる敵には反映されない。
- `airStayTime` は全敵共通なのでCSVから外し、`EnemyAirStay.cpp` の定数のまま。
- Idle後の抽選は1回にまとめた(`IsHitRate(chaseRate)`)。前は rand を2回引いていて実質 Chase75%/Caution25% だった。
- `EnemyKnockBack` はどこからも使われていない(knockBackTime も今は効いていない)。
- 方針: 種類ごとの標準値はこのCSV。Unityの配置CSVには「この1体だけ変えたい値」(HP・視界など)だけを上書き用の列として後で足す(空欄なら標準値)。

### 2026-10-12 被弾後の確定ヒット(ガードしない段数)
- `EnemyStatus.csv` の guardBreakTime の後に `forceHitCount`(3) / `forceHitResetTime`(60F) を追加。
- `EnemyBase::OnDamage`: ダメージが通ったら `m_forceHitCount = forceHitCount`。残りがある間の弱攻撃はガード抽選をせず1減らして当てる。
  確定ヒットで当たった分や、残りがある間の強攻撃では補充しない(使い切ると次の弱攻撃から抽選に戻る)。
- `EnemySwordman::Update`: 最後の被弾から `forceHitResetTime` たつと残りを0にする。

### 2026-10-12 敵の攻撃CSVに attackFinishFrame を追加(読み込みまで)
- `EnemySwordmanAttack.csv` の animEndFrame の後に `attackFinishFrame`(攻撃を終えて後隙へ移るフレーム、空欄なら-1=アニメの最後まで)。
- `EnemyAttackNode::attackFinishFrame` / `EnemyAttackNodeType::AttackFinishFrame` を追加し、`InitializeAttackList` で読む。
- **まだ使っていない**。EnemyAttack は今もアニメ終了(GetAnimEndFlag)で次へ移る。

### 2026-10-13 敵の攻撃権(AttackTokenManager)
- `Managers/AttackTokenManager.h/.cpp`。**シングルトンではなく BattleManager が `shared_ptr` で持つ**。`BattleManager::Init()`(SceneMain::Init で呼ぶ)で作る。
  `GetAttackTokenMgr()` は weak_ptr を返すので、`System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock()` して使う。BattleManager はシーンごとに作り直すので、前のシーンの攻撃権は残らない。
- 中身: 同時に使える重さの合計 `m_maxCost`(2)、攻撃ごとの重さ `cost`(省略で1)、全体の攻撃間隔 `kGlobalAttackInterval`(30F。誰かが取ってから次に渡すまで)。
  名簿 `m_holders` は weak_ptr + cost。比べるのは `GetId()`(Collider のID)。
- 取る: `NextAfterIdle` / `NextOnArrived` で、攻撃を選べたときだけ `Request`。取れなければ今まで通り Chase/Caution(Caution → Chase → 着いたらまた頼む、のループで待つ)。
- 返す: `EnemyBase::ChangeState` で「攻撃の流れ(AttackPre/Attack/AttackEnd)から、それ以外に移ったとき」に `Release`(被弾・処刑などの中断も含む)。返すのは AttackEnd → Back のとき。
  - 「攻撃の流れから移ったとき」だけにしているのは、敵の Init の最初の ChangeState が BattleManager を作る前(SceneMain::Init)に呼ばれ、`GetBattleMgr` の assert に当たるため。
- 保険: `AttackTokenManager::Update`(`BattleManager::Update` の中で呼ぶ)で、消えた敵・死んだ敵の分を回収する。
- `BattleManager::Update` は System::Update の中ではなく、`SceneMain::NormalUpdate` の `System::Update()` の直後で呼ぶようにした(Systemでタイムスケールを戻した後に更新するため)。
- `NextAfterParry`(弾き後の反撃)は攻撃権を見ずに攻撃する(ひとまずこのまま)。持っていなくても攻撃後の Release は何もしないだけ。
- EnemyAttackPre の攻撃権の TODO は消した。
- デバッグ表示(F3): 攻撃権を持っている敵は足元に紫の輪＋頭の上に「TOKEN」。左上(10,130)に `AttackToken:使用/上限 Interval:残り`。

### 2026-10-14 フォルダ整理(案A)
- **これより前のログに出てくるパスは整理前のもの**。対応表(全ファイル・全include)は `../進捗/ファイル整理_変更一覧.md`。
- 今の構成:
  - `Main/` Application, Game, System, Input, IDManager, imguiApp, main
  - `Scene/` SceneMain(PauseScene などと同じ場所)
  - `Battle/` BattleManager, AttackTokenManager
  - `Collision/` 旧 Collider/ の全部 ＋ CollisionManager, CollisionChecker, FixNextPosition ＋ AttackCol, HitCol, JustDodgeCol, SkillAttackCol
  - `Character/` CharacterBase, Animation
    - `Player/` Player, PlayerState, PlayerEnums, PlayerWeapon(旧 Base/ を直下へ)。`Player/State/{Attack, Defence, Locomotion}`
    - `Enemy/State/` EnemyStateBase。`Locomotion/`(Idle, Chase, Patrol, Caution, Back)、`Defence/`(Guard, GuardBreak, Parry)、
      `Hit/`(AirFall, AirStay を追加)、`Attack/`(変更なし)。General/ は廃止
  - `Render/` ModelShaderManager, EffectManager, BloodKillFog。`Render/Shader/`(旧 Shader/)
  - Camera/, Stage/, UI/, Math/, DataLoader/ は変更なし。Managers/, Collider/, Shader/ は廃止
- 追加のインクルードディレクトリは `$(ProjectDir)` にした(Player の各フォルダと Character は外した)。
  **新しい include はプロジェクト基準で書く**(例 `#include "Main/System.h"`)。同じフォルダならファイル名だけ。
- シェーダーは実行時に読むので、`ModelShaderManager.cpp` のパスを `Render/Shader/...` に変えた。
- Debug/Release(x64) とも Rebuild が通ることを確認済み。ゲームの起動確認はまだ。

### 2026-10-16 フェーズ進行 Step1(PhaseManagerへ移すだけ)
- `Battle/PhaseManager.h/.cpp` を追加し、vcxproj/filters に登録。`SceneMain` が `shared_ptr` で持つ(EnemyManager は weak_ptr で参照)。
  - `Init(enemyManager, spawnData)`: 配置データのフェーズ番号を集め(`m_phaseList`、小さい順・重複なし)、EnemyManager に `SetSpawnData` して最初のフェーズを始める。
  - `Update()`: `SceneMain::NormalUpdate` の `m_enemyManager->Update()` の直後で呼ぶ。全滅していたら `ChangePhase(FindNextPhase())`。
  - `m_currentPhase`: 0=始まっていない、-1=全フェーズ終わった。
- EnemyManager に残したもの: `SetSpawnData` / `StartPhase(phase)`(public にした) / `IsPhaseEnemiesAllDead()`。
  `StartSpawn` / `UpdatePhase` / `FindNextPhase` / `m_currentPhase` / `GetCurrentPhase` は削除。
- Debug(x64) のビルドは通った。**ゲームでの確認はまだ**(全滅で次の敵が出るか)。
- 新しいファイルは **UTF-8(BOM付き)・CRLF** で保存すること(他のファイルに合わせる)。

### 2026-10-16 フェーズ進行 Step2 の1〜3(遷移表の読み込みまで)
- `Stage/PhaseFlowData.h`: `PhaseConditionType` / `PhaseEvent` / `PhaseTransition`(1行分)。vcxproj/filters に登録。
- `DataManager::LoadPhaseFlow(stageNumber)` / `GetPhaseFlow()` / `GetPhaseFlowFilePath()`、`StringToPhaseCondition` / `StringToPhaseEvent`。
  - パスは `data/Stage/PhaseFlow/PhaseStage003.csv`。event列は省略可(None)。知らない condition/event の行は飛ばす。
  - Timer/KillCount の `intParam`、EnterArea の `areaIndex` はまだ読んでいない(Step4/5)。
- `data/Stage/PhaseFlow/PhaseStage003.csv` に今と同じ流れ(0→1→…→6→-1、全部 AllDead)を書いた。
- Step2の4(Initまで): `SceneMain::Init` で `LoadPhaseFlow` して `PhaseManager::Init(enemyMgr, spawnData, flow)` に渡す。
  - 遷移表が空・Start の行が無いときは assert。`from=0, condition=Start` の行で `ChangePhase(to, event)`。
  - `m_phaseList` / `FindNextPhase` は削除。`ChangePhase` に event 引数を追加(処理はまだ)。
  - Update / IsConditionFulfilled / ChangePhase はユーザーが書いた(Start と AllDead(param空)のみ対応)。
    ChangePhase は to=-1 でも event を処理する(敵を出すのは next>0 のときだけ)。

### 2026-10-16 フェーズ進行 Step3(デバッグ表示)
- `EnemyManager::m_totalKillCount`: `UpdateCorpses` で新しく死んだ敵を死体リストに入れるときに+1。`GetTotalKillCount()`。
- PhaseManager: `m_phaseFrame`(実フレーム、Updateで+1)、`m_killCountAtPhaseStart`(ChangePhaseで記録)、
  `GetPhaseKillCount()` = 合計 − フェーズ開始時。前のフェーズの生き残りを倒した分も数える。
- `PhaseManager::DebugDraw()`: (10,150) に `Phase / Frame / Kill`。`SceneMain::NormalDraw` の `#ifdef _DEBUG` で EnemyManager の DebugDraw の次に呼ぶ。
- ビルドは通った。**ゲームでの確認(Step2・3とも)はまだ**。

### 2026-10-16 フェーズ進行 Step5 の2(エリアCSV)
- `data/Stage/PhaseAreas/Stage003.csv` を追加。列は `#name,posX,posY,posZ,halfExtentX,halfExtentY,halfExtentZ,rotYDeg`(壁ゾーンから type/isActive を抜いた形)。
  - `Area_Test`(0,0,4000 / 500,300,300): 確認用。フェーズ1の敵の近く(床 y=0 あたり)。
  - `Area_H0` / `Area_Boss`: 設計メモの例の値そのまま。実際の地形に合わせて直す。
- position は箱の中心。足元の高さが 中心±halfExtentY に入らないと判定されない。

### 2026-10-16 フェーズ進行 Step5 の3(エリアCSVの読み込み)
- `Stage/PhaseAreaData.h`: `PhaseArea`(name / position / halfExtents / rotYDeg)。vcxproj/filters に登録。
- `DataManager::LoadPhaseAreas(stageNumber)` / `GetPhaseAreas()` / `GetPhaseAreaFilePath()`。列番号は `PhaseAreaColumn`(無名namespace)。
  - **try を使わない形**。数値の欄が空欄・文字だと `std::stof` の例外で止まる(どの行かは出ない)。列不足の行は飛ばす。
  - 敵配置の `EnemySpawnColumn::RotYDeg` と名前がぶつかるので、列名は `AreaName` / `AreaPosX` … `AreaRotYDeg` にした。
- **まだどこからも呼んでいない**。次は SceneMain で読んで PhaseManager::Init に渡す(手順4〜)。
- (2026-10-17 確認) Step4 はコードに入っていた: `IsConditionFulfilled` で AllDead の param(グループ指定 → `EnemyManager::IsGroupAllDead`)・Timer(`m_phaseFrame`)・KillCount(`GetPhaseKillCount`)、`LoadPhaseFlow` で `intParam` を読む。
- ここではビルドできないので、DxLib を仮の定義にして DataManager.cpp の文法だけ確認した。VS でのビルドはまだ。

### 2026-10-16 Stage フォルダの整理・使っていない箱CSVの削除
- 削除: `StageCsvIO.h/.cpp`、`StageObjectData.h`、`StageObject.h/.cpp`(どこからも使われていなかった BOX 配置CSV)。
  `Stage` の `LoadStageObjects` / `ClearStageObjects` / `m_stageObjects`(と Draw・デストラクタの呼び出し)も外した。
- 今の構成:
  - `Stage/` Stage, StageInfo, SkyBox(SkyBox は今どこからも使われていない)
  - `Stage/WallZone/` WallZone, WallZoneData, WallZoneEditor
  - `Stage/Phase/` EnemySpawnData, PhaseFlowData, PhaseAreaData(CSV 1行分の構造体)、PhaseAreaEditor(10/17追加)。PhaseManager は `Battle/` のまま
- 直した include はプロジェクト基準で書いた(例 `"Stage/Phase/EnemySpawnData.h"`、`"Math/Vector3.h"`)。vcxproj/filters のパスも変更。
- ここではビルドできないので、include のパスが全部見つかることだけ確認した。**VS でのビルドはまだ**。

### 2026-10-17 フェーズ進行 Step5 の4〜(EnterArea・エリアエディタ)
- `PhaseManager::Init(enemyMgr, player)`: 遷移表・エリア・敵配置は Init の中で DataManager から取る(Load は SceneMain で先に済ませる)。
- 名前→番号は `ResolveAreaIndex()`(見つからなければ -1)。Init では `GetMissingAreaNames()` が空でなければ assert。
- `IsInArea`: `BoxShape::WorldToLocalDir` でプレイヤーの位置を箱のローカルに直して比べる。足元 = `GetRigidBody().GetPos()`。
- `DrawAreas(selected)`: 黄=通常、赤=プレイヤーが中、白=選択中。Debug ビルドでは普段から描く(編集モード中はエディタ側で描く)。
- `Stage/Phase/PhaseAreaEditor`(WallZoneEditor とほぼ同じ作り): ステージ編集モードで「Phase Area Editor」ウィンドウ。
  追加(カメラ注視点/プレイヤー足元)・複製・削除・名前/位置/大きさ/回転・床に落とす・Undo/Redo・Load/Save(`DataManager::SavePhaseAreas`)。
  - 編集はその場で `PhaseManager::SetAreas` に入る(番号も引き直す)。遷移表で使っている名前が無くなると赤字で出る。
  - Ctrl+Z/Y は触っているウィンドウだけ(WallZoneEditor にも同じ条件を足した)。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-17 フェーズ終了でリザルトへ遷移
- `BattleManager::SetIsBattleEnd / GetIsBattleEnd` を追加。`PhaseManager::ChangePhase` で next=-1 のときに立てる。
- `SceneMain::NormalUpdate` の最後でフラグを見てフェードアウト → `FadeOutUpdate` で `ResetScene<ResultScene>()`。
  PhaseManager から直接 ResetScene しないこと(Update 中に SceneMain が破棄される)。
- `Scene/ResultScene.h/.cpp`(仮、文字だけ)。Aで `StageSelectScene` へ。vcxproj/filters に登録。
- 注意: PhaseManager::Init は BattleManager を作る前に呼ばれるので、Start の行で直接 -1 に行くと GetBattleMgr の assert に当たる。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-17 敵のデバッグ表示にグループ名
- `EnemyBase::DebugDraw`: `m_groupId` が空でなければ、ステート名の上(TOKEN があればさらにその上)に緑で `Group:〜` を出す。

### 2026-10-17 ガードブレイクのチャンス状態(PlayerStateGuardBreakChance)の土台
- バトルデザイン: 弱→強はガード崩し(打ち上げない)。ブレイクしたら自動でチャンス状態に入り、時間内にYで打ち上げ派生(案A)。
- `Character/Player/State/Attack/PlayerStateGuardBreakChance.h/.cpp` を追加し、vcxproj/filters に登録。Player.h に include と friend。
- 流れ: `EnemyGuardBreak::Enter` → `player->RequestGuardBreakChance(owner)`(通知だけ) → 次の `Player::Update` の
  `UpdateGuardBreakChance()`(State更新の前)で遷移。敵から直接 ChangeState しないのは、当たり判定の処理中に攻撃判定を消さないため。
  - 入る条件: 今のStateが `PlayerStateAttack` かつ地上。通知は毎回消す(条件を満たさなくても持ち越さない)。
  - ターゲットは State のコンストラクタで `player->m_guardBreakChanceTarget` から受け取る。そのため reset は ChangeState の**後**。
- 中身(仮): Idleアニメ、その場で止まる、`kChanceTime`(60F)で Idle/Move へ。敵が消えた・GuardBreakが終わったら終了。
  Bで回避。Yは **TODO(今は Idle/Move に戻るだけ)**。
- TODO: 打ち上げ攻撃のステート、専用カメラ(Enter/Exit)、スロー・演出。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

---

### 2026-10-17 打ち上げ攻撃ステート(PlayerStateGuardBreakAttack)の土台
- `Character/Player/State/Attack/PlayerStateGuardBreakAttack.h/.cpp`(`PlayerStateAttackBase` 継承)を追加し、vcxproj/filters・Player.h(include/friend)に登録。
- `PlayerStateGuardBreakChance` で Y を押すとここへ遷移する。

### 2026-10-17 打ち上げ攻撃(PlayerStateGuardBreakAttack)の中身
- **スキル0(index 15) → 切り上げ(index 7) → 先行入力があれば PlayerStateAttack(空中攻撃1)**。
  2段で1つの演出なので自分に遷移せず、State内の `Phase`(Skill0 / UpAttack)で切り替える(Enter/Exit は1回ずつ)。
  - `StartPhase`: 前の判定を消す → `currentComboIndex` をその段に → アニメ → (切り上げのみ RootMotionCancel::Up) → `InitVerticalMove` → `CreateAttackCol`。
  - Skill0: 当たったらカメラの注視点を敵に(`m_isCameraFocusOverride`。SkillAttack の UpdateLaunchCamera と同じ)。cancelFrame で当たっていれば切り上げ、外れたら Idle/Move。
  - UpAttack: X/Y の先行入力 → cancelFrame で `PlayerStateAttack(弱/強)`。なければアニメ終了で Fall(地上なら Idle/Move)。回避・ジャンプのキャンセルは無し。
  - Exit: currentComboIndex=None、isHit/鴉/カメラ注視点を戻す、判定を消す。
- スキルゲージは消費しない(ブレイクのご褒美)。振りのSEは PlayerStateAttack 側も全部コメントアウトなので入れていない。
- 打ち上げの量は CSV の knockBack のまま。**打ち上げの処理・CSVはユーザーが作り直す予定**。
- (追記) スキル0・切り上げの行を使い回すのをやめ、`ComboChain01.csv` の最後に専用の行を追加した。
  `ガードブレイクアタック0`(index 20、スキル0のコピー) / `ガードブレイクアタック1`(index 21、切り上げのコピー)。どちらも attackType=4(GuardBreakAttack)、nextLight/nextHeavy は空欄。
  `ComboIndex::GuardBreakAttack0/1` を追加。判定は通常の `CreateAttackCol`(attackType は CSV から入る)。
- `EnemyGuardBreak::CanChangeHit` の `else true;` → `else return true;` に直した(C4715 が消えた)。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-18 銃の敵(EnemyGunner)
- `Character/Enemy/EnemyGunner.h/.cpp`(EnemySwordman のコピー)。モデルは `data/Enemy/EnemyGun.mv1`(`AsyncData::EnemyGunnerModel`)。vcxproj/filters に登録。
  - 部位破壊で落とすパーツのモデルが無いので、パーツ関係(Z/Xのデバッグ部位破壊も)は外した。`OnHeadBreak` はフラグを立てるだけ。
  - Q で `EnemyGunnerAttack.csv` とステータスを読み直す。
- CSV: `data/Enemy/EnemyGunnerAnim.csv`(Rifle系のアニメを割り当て)、`EnemyGunnerAttack.csv`(仮の「銃床打ち」1行、Rifle Punch。値は適当)、
  `EnemyStatus.csv` に `Gunner` 行(Swordmanの値をもとに HP600、leftArmFrame=16、headFrame=40)。
- `EnemyType::Gunner` / 配置CSVの type `Gunner`。`DataManager::LoadRawCsv` を作り、Swordman と Gunner の攻撃CSVの読み込みを共通にした。
- 確認用に `Enemies003.csv` の最初の3体(フェーズ1・A2)を Gunner にした。Unityの `EnemySpawnType` にも Gunner を追加
  (**Unityから配置を書き出し直すと Swordman に戻る**ので、Unity側のその3体も Gunner にしておくこと)。
- 撃つ処理はまだ無い(近接の Swordman と同じ動き)。MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。
- 銃: `Character/Enemy/EnemyGunnerWeapon.h/.cpp`(PlayerWeapon と同じ作り)。`data/Enemy/EnemyGun/AK74.mv1`(`AsyncData::EnemyGunnerWeaponModel`)を
  右手(`mixamorig:RightHand`)の行列に合わせる。EnemyGunner の Init で生成、Update の最後で更新、Draw で描画(血殺中は黒)、SetOpacity で一緒に消える。
  手元の位置・回転は Rifle Idle の姿勢で計算して合わせた(銃口=右手→左手の向き、握り=手首と人差し指・中指の付け根の中間)。
  走り・歩きでも構えた形になる。Gunplay(銃を回す)や Rifle Punch では左手が離れる。
- 部位破壊: Swordman と同じ処理(Z/Xのデバッグ部位破壊も)。頭は Swordman と同じ `Part/Swordman/Head.mv1`(`EnemyPartHeadModel`)。
  左腕は `data/Enemy/Part/Gunner/LeftArm.mv1`(`EnemyGunnerPartLeftArmModel`)。Swordman の LeftArm.mv1 から盾のメッシュを外したもの。
  - 作り方: DxLib で元の mv1 の腕メッシュ(mesh 1)を参照メッシュで取り出し(フレームの変換込み)、`LeftArm.x` に書いて読み直し MV1SaveModelToMV1File。
    テクスチャは同じフォルダの Meshy_AI_〜.png(元の mv1 が実際に使っている白青のもの)。LeftArm.x は元データとして残してある。
  - フレームの変換を頂点に焼き込んだので、`leftArmModelScale`(0.01)は Swordman と同じ値のまま。
  - 元の LeftArm.fbx はメッシュ名が逆(`Helmet.001` が盾、`Sword` が腕)。

### 2026-10-18 バズーカの敵(EnemyBazooka)
- EnemyGunner と全く同じ作り。`Character/Enemy/EnemyBazooka.h/.cpp`・`EnemyBazookaWeapon.h/.cpp`(Gunner のファイルの名前を置き換えたもの)。
  体のモデル(`EnemyGunnerModel`)・盾なしの左腕(`EnemyGunnerPartLeftArmModel`)・頭はガンナーと共有。武器だけ RPG7(`AsyncData::EnemyBazookaWeaponModel`)。
- CSV: `EnemyBazookaAnim.csv` / `EnemyBazookaAttack.csv`(Gunner のコピー)、`EnemyStatus.csv` の `Bazooka` 行(Gunner と同じ値)。type は `Bazooka`(Unity の EnemySpawnType にも追加)。
- 確認用に `Enemies003.csv` の4・5体目(フェーズ1・A3)を Bazooka にした。
- RPG7 のモデル: `data/Enemy/EnemyBazooka/RPG7.mv1`(元は `Desktop/GunFBX/Gun/RPG7.fbx`)。
  - DxLib は FBX を読めないので、Blender で .x に書き出して(スクリプトは scratchpad の fbx2x.py)DxLib で読み直して mv1 に保存した。`RPG7.x` は元データとして残してある。
  - 軸はビューアの変換と同じ(x=x, y=Blenderのz, z=Blenderのy)。原点は引き金。AK74 と同じく +Z が前(弾頭)、+Y が上。
  - RPG7.fbx は AK74.fbx の10倍の大きさで作られていたので、倍率は ×10(AK74 は ×100)。全長は弾頭込みで約127。
  - 弾頭は `Rocket`、発射筒は `Launcher` という別のフレームにしてある(撃ったときに弾頭だけ消す用)。
  - 手元の値はガンナーと同じ(銃のモデルの向きをそろえたので同じ値で合う)。銃のアニメのままなので、肩に担がずライフルと同じ持ち方。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-19 暗殺・確殺の距離制限
- `LockOnManager.cpp` 先頭に `kAssasinRange` / `kPartBrokenRange`(どちらも400・仮)と `kExecuteReleaseMargin`(50)。
- `TryAssasinTarget` / `TryPartBrokenTarget`: 探す範囲を `範囲 * i / kLockOnCheckNum`(400なら133→266→400)にした(前はロックオンと共通の300×i、最大900)。
- `CheckAssasinTarget` / `CheckPartBrokenTarget`: 外す距離を `kLockOnMaxDistance`(10000)から `範囲 + kExecuteReleaseMargin` にした。
- デバッグ表示 `LockOnManager::DebugDraw()`(SceneMain の `#ifdef _DEBUG` で呼ぶ): プレイヤーの足元に円(暗殺=紫、確殺=赤。明るい=入る距離、暗い=外れる距離)、
  対象の頭の上に球(暗殺=紫、確殺=赤。確殺の球は少し上)。範囲が同じ値だと円は重なる。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-19 確殺・暗殺で頭を切ったときに血を出す
- `EnemyBase::PlayBlood(effect, pos)`: `PlayBloodSplash` から再生部分を切り出したもの(+Z をプレイヤー→敵の向き、スケール20)。再生ハンドルを返す。
- `EnemyBase::PlayHeadBreakBlood()`: 首の付け根(`m_status.headFrame` の位置)から `BloodSplash_B`(真上に噴き上がる)を出す。
- `OnHeadBreak()` を呼んでいる3か所(`EnemyPartBrokenKilled` のパターンA/B、`EnemyAssasined`)の直後で呼ぶ。暗殺でも出る。
- 斬るフレームと頭が飛ぶフレームがずれて見えたら、血用のフレームを別に持つ(`m_anim.IsAnimFrameOver` で判定)。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-19 鴉状態(m_isRaven)を削除
- `Player::m_isRaven` / `GetIsRaven` と、鴉の羽(`m_wingModelHandle` / `WingUpdate` / `kWingScale` / `kWingOffset`)を削除。
  Idle/Move/FastRun の「LB を押している間は鴉状態」も消した(LB は今は何もしない)。
- `AttackCol::PlayerGaugeUp` は鴉状態の代わりに `m_attackData->attackType`(SkillAttack / GuardBreakAttack)でスキル攻撃を判別する。ゲージの増え方は前と同じ。
- `AsyncData::PlayerWingModel` の読み込み・解放(System)は残っている。要らなければ消す。
- MSBuild(Debug x64)でビルドは通った。

### 2026-10-19 血殺の発動をスキル1・2の最後の一撃に限定
- `AttackData` に `isLastHit` / `comboIndex` を追加。`CreateAttackCol` で comboIndex、`UpdateAttackWindow` の新しい区間で `SetIsLastHit(最後の区間か)`(単発も true)。
- `AttackCol` の血殺条件: `GetIsUltimating() && isLastHit && comboIndex が SkillAttack1/AirSkillAttack1/SkillAttack2`。attackType は見なくなった。
- 同じ if の中にある被ダメージのエフェクトの向きの処理も、最後の一撃のときだけになる(今はエフェクトがコメントアウトされている)。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。
- (追記) スキル3の着地の衝撃(`m_burstAttackCol`)でも血殺。`AttackType` に `ImpactAttack`(5) / `ImpactSkillAttack`(6) を追加し、
  `InpuctAttackSetUp` で今の段の attackType が SkillAttack なら ImpactSkillAttack、それ以外(空中強攻撃の着地)は ImpactAttack。comboIndex・isLastHit=true も入れる。
  前は None だったが、敵側の attackType の判定(弱でガード・スキルでガードしない等)はどちらにも当たらないので挙動は同じ。

### 2026-10-20 ボス(BossEnemyPirate)
- `Character/Enemy/BossEnemyPirate.h/.cpp`(EnemySwordman のコピー、名前だけ置き換え)。vcxproj/filters に登録。
  - モデル: `data/Enemy/BossPirate/BossPirate.mv1`(既にあった `AsyncData::BossModel` で読み込む)。UE の骨格(腰=`pelvis`、左腕=38 `upperarm_l`、頭=78 `head`)。テクスチャ無し。
  - 部位破壊で落ちるパーツは Swordman のもの(盾付きの腕・ヘルメット)を仮で使っている。
  - 大きさ: `CharacterBase::m_modelScale`(新規、標準1.0)を `UpdateAngleAndPos` で掛ける。ボスは `kModelScale`=1.3(身長 約187→243)。
- CSV: `BossEnemyPirateAnim.csv`(Mixamo名の9個だけ別名。残りは Swordman と同じ名前)、`BossEnemyPirateAttack.csv`(Swordman のコピー)、
  `EnemyStatus.csv` の `BossEnemyPirate` 行(HP3000、当たり判定を約1.5倍、視界1000)。`EnemyType::BossEnemyPirate` / 配置の type `BossEnemyPirate`。
- 配置: `Enemies003.csv` 最後の行。ボス広場の台の上(Unity の Boss マーカー 0,-11.7,315m → 0,-1170,31500)、180度(-Z、来る方を向く)、phase7、Idle、group `Boss`。
  **Unity から配置を書き出し直すと消える**ので、Unity の EnemySpawnType にも追加すること。
- フェーズ: `5 AllDead→6`、`6 EnterArea(Area_Boss)→7 / BossAppear`、`7 AllDead→-1 / GameClear`(イベントの中身はまだ空)。
  `Area_Boss`(0,-1100,29800 / 2400,300,500): 坂を下りきった先、広場の入口。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**(首の向きの処理が UE の骨格で変に回らないか、攻撃の距離・判定が大きさに合うか)。
- (追記) テクスチャ: Unity の `Desktop/programm/ninja_motion/Assets/Pirate/Textures` の Albedo(4096の TGA)を 2048 の PNG にして `data/Enemy/BossPirate/Texture/` に置き、
  `BossPirate.mv1` に貼って上書き保存した(DxLib の MV1AddTexture → MV1SetMaterialDifMapTexture → MV1SaveModelToMV1File。元の mv1 は git にある)。
  - 割り当ては Unity のプレハブ `Pirate_Full` と同じ: 体(Body_01)・服(Cloth)・小物(Details_Weapons)・髭(Hair_01)。服と髭はアルファテスト(>128)。ディフューズ色は 1.0 にした。
  - 法線・AO・メタリックは使っていない。体・髭の色違い(_02/_03)も未使用。
- (追記) デバッグ: **F4でボスの一つ手前のフェーズへ飛ぶ**(Debug ビルドのみ。`SceneMain::NormalUpdate`)。
  `PhaseManager::DebugSkipToBossPrevPhase()`: 配置CSVでボスのいるフェーズを探し、そこへ進む行の from へ `ChangePhase`(event は None)。
  `EnemyManager::DebugClearEnemies()` で敵・死体を全部消す。その行が EnterArea なら、プレイヤーをエリアの手前(-Z側に halfZ+300、高さは箱の中心)へ移して速度を0にする。
  ゲームでの確認はまだ(カメラが遅れて追いかけないか)。

### 2026-10-20 スキル血殺用のカメラ(SkillKessatuCamera)
- `Camera/CameraState/SkillKessatuCamera.h/.cpp`(UltCameraState のコピー、名前だけ置き換え)。vcxproj/filters に登録。
  `CameraStateName::SkillKessatuCamera` / `CameraStateBase::Type::SkillKessatuCamera`(19)。
- `AttackCol` のカメラ移行: `isUltimating`(スキル1・2の最後の一撃/スキル3の着地)なら SkillKessatuCamera、それ以外(UltAttack)は UltCamera。
- コンパイルは通った(リンクはゲーム起動中で未確認)。ゲームでの確認はまだ。
- (追記) 長さの定数も分けた(`AttackCol.cpp` 先頭): `kSkillKessatuFrame`(演出の長さ) / `kSkillKessatuSlowFrame`(スローの長さ) / `kSkillKessatuTimeScaleRate`。
  初期値は必殺技と同じ(120 / 120 / 0.1)。`SetUltStart` も分岐の中に入れた。
  - 注意: 演出の長さ(`m_ultCount`)は `BattleManager::Update` でタイムスケールを掛けて減るので、スロー中はほぼ減らない。
    今の値だと演出は実フレームで約228F(スロー120F + 等速108F)。カメラ・フォグ・黒い敵・UIは演出の長さに従う。
- MSBuild(Debug x64)でビルドは通った。

### 2026-10-20 m_isPlayerFound の初期値を true に
- `EnemyBase::m_isPlayerFound` の初期値を true にし、`SetIsPlayerFound()` を追加。
- `EnemyManager::CreateEnemy` で Patrol / Watch の配置のときだけ false にする(デバッグ用の `SpawnEnemy` も false)。Idle 配置の敵は最初から発見済み(暗殺できない)。
- 配置の初期状態 `Guard` を `Watch` に改名(ガードのStateと紛らわしいため)。`SetGuardPoint` → `SetWatchPoint`、配置CSVの文字列も `Watch`(Enemies003.csv / TestStage/Stage003.csv)。
  **Unity の書き出し側も `Watch` に直すこと**(`Guard` のままだと読み込みでその行が飛ばされる)。
- ビルドはまだ。

### 2026-10-20 空中暗殺(PlayerStateAirAssasin / EnemyAirAssasined)
- `PlayerStateAssasin` / `EnemyAssasined` のコピー。アニメだけ変更: プレイヤー `Assasin02`、敵 `fromAvobeForwardVictim`。vcxproj/filters・Player.h・EnemyBase.h(include/friend)に登録。
  - 4段階(プレイヤー・敵とも同じフレームで進む。各定数は .cpp 先頭、敵側の値はプレイヤーと合わせる):
    Approach(11F、敵の手前50へlerp) → Move(25F、2人とも水平に速度8で移動) → Execute(敵は頭破壊＋血、プレイヤーは止まって25F) → End(アニメ終了で終わり)。
  - フレームはタイムスケールを掛けたゲームのフレーム(アニメのフレームではない)。カメラは Approach → Move で AssasinCamera に切り替え。
- `EnemyBase::OnAirAssasined()`(OnAssasined と同じく処刑済みにして EnemyAirAssasined へ)。
- 入口: `PlayerStateJump` / `PlayerStateFall` で `Y && CanAssasin()`(空中攻撃より前)。
- 対象は `m_assasinTarget` を共通で使う。`LockOnManager` で、プレイヤーが空中(`!IsFloor()`)なら範囲を `kAirAssasinRange`(800・3D距離)にする(探す・外す・デバッグの円)。
  見つかっている敵・空中の敵を外す条件は地上と同じ。着地すると地上の範囲(400+50)で外れる。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。
- (追記) 空中暗殺用のカメラ `Camera/CameraState/Assasin/AirAssasinCamera`(`AirAssasinCameraState`) / `AirAssasinCameraStart` を追加(AssasinCamera / AssasinCameraStart のコピー、名前だけ置き換え)。
  `CameraStateName::AirAssasinCamera / AirAssasinCameraStart`、`Type` 20 / 21。vcxproj/filters に登録。
- (追記) `PlayerStateAirAssasin::Enter` で `AirAssasinCameraStart` に切り替える。中身を UltCamera 風に変更:
  - 位置: 敵→プレイヤーの延長線から5度、Enter時のカメラに近いほうへずらす。水平距離300(PlayerFollowは350)、高さ250。注視点はプレイヤーから敵側へ100・高さ100。
  - 視野角: 60°→40°へ45Fでだんだん狭くする(EaseOut、タイムスケール込み)。戻し方は下の「視野角をカメラの状態として扱う」。
  - 遷移は地上の暗殺と同じ: `PlayerStateAirAssasin` の Enter で Start、Approach→Move で `AirAssasinCamera`(Startからは上書きブレンドでプレイヤー中心の Slerp・EaseOutBack 40F)、
    Start 自身のタイマーでの遷移はコメントアウト。
  - `AirAssasinCamera` の中身は PartBrokenACameraEnd と同じ: プレイヤーの向き(`GetTargetVec`)の後ろ、PlayerFollowCamera の初期値(距離350・縦 π/10・注視点の高さ150)へ向かい、
    ブレンドが終わったら `PlayerFollowCamera`。Exit で `m_goalAngleH/V` を渡し、上書きブレンドは None。前の中身(敵側から見上げる・距離を縮める)は消した。
  - Start にいるのは Approach の間だけ(11F)なので、視野角は 40° まで狭まりきらない。狭めきりたいなら `kZoomFrame` を短くする。
  - MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-20 視野角をカメラの状態として扱う
- `CameraStateBase` に `m_fov / m_startFov / m_goalFov`(初期値 `kDefaultFov` = 60°)。演出したいカメラは `m_goalFov` を変えるだけ。`GetFov()` はブレンドが終わっていれば目標値を返す。
- 引き継ぎ: `CameraData::fov`。`CameraManager::ChangeState` で前のカメラの `GetFov()` を詰め、`SetStartFov(data.fov)` を **Enter の前に**呼ぶ。
- ブレンド: `UpdateBlend` で位置と同じ t を使い、`BlendSetting::fovEasingMode / fovEasingPower`(最後に追加、初期値 EaseOut・2.0)で補間する。
  カット切り替え(`m_activeBlend.mode == None`)のときはすぐ目標値。使われる設定は位置と同じ(新しいカメラの GetBlendSetting か、前のカメラの GetOverrideBlendSetting)。
- 反映: `CameraManager::Update` / `ApplyCameraSettings` で `SetupCamera_Perspective(GetFov())`。
  フォトモード(ステージ編集中も)・Override Camera は Camera Debug ウィンドウの `FOV (deg)`(`imguiApp::GetOverrideCameraFov`、10〜120°、Reset FOV で60°)。カメラアニメーターは `kDefaultFov`。
  Camera Debug には今の視野角(`Current FOV`、DxLib の GetCameraFov)も出す。
- 各カメラの `SetupCamera_Perspective` と `kCameraViewAngle` は消した。**新しいカメラでも SetupCamera_Perspective を呼ばないこと**。ニア・ファー(`SetCameraNearFar`)は各カメラのまま。
  `SceneMain` の初期化と、古い `Camera.cpp` の SetupCamera_Perspective は残っている(CameraManager が毎フレーム上書きする)。
- AirAssasinCameraStart: 視野角はブレンド設定だけで変える。Enter で `m_goalFov` = 40°。
  60°→40° は Start の `GetBlendSetting`(30F、`kFovEasingMode/Power`)、40°→60° は Start の `GetOverrideBlendSetting`(40F、`kNextFovEasingMode/Power`)。どちらも初期値 EaseOut・2.0。
  長さは `kFovDuration`(45F) / `kNextFovDuration`(40F)。ブレンドはタイムスケールに関係なく実フレームで進む。
- `BlendSetting::fovDuration`(負の値なら duration と同じ。`GetFovDuration()`)。視野角だけ長さを変えられる。
  `IsBlending()` は duration と fovDuration の長いほうまで true(視野角のほうが長いと、位置は着いていてもブレンド中扱い。ブレンドが終わったら次へ移るカメラはその分遅れる)。
  コンパイルは通った(ゲーム起動中でリンクは未確認)。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ(空中暗殺の後のカクつきが消えたか)。

### 2026-10-22 FOVパンチ
- `CameraManager::StartFovPunch(angleDeg, time)`(+で広げる、-で狭める) / `FovPunchUpdate()`(今フレームのずれ、ラジアン)。
  - 入りは `kFovPunchInFrame`(2F)で最大へ、戻りは EaseOut(`kFovPunchOutPower`)で0へ。**実フレーム**で進む(ヒットストップ・スロー中も止まらない)。
  - 今のずれより弱いパンチは無視(多段ヒットで強くなりすぎないように)。
  - クールタイム `kFovPunchCoolTime`(20F、実フレーム)。受け付けたときだけ入り、その間は強さに関係なく全部無視。`FovPunchUpdate` の先頭で減らす。
- `Update` で `m_renderFov = GetFov() + FovPunchUpdate()` を1回だけ計算し、`ApplyCameraSettings` は `m_renderFov` を使う(位置の `m_renderPos` と同じ)。
- 呼び出し: `AttackCol` のカメラシェイクと同じ場所(最初に当たったとき・必殺技以外)。`kFovPunchAngle`(4°) / `kFovPunchTime`(14F)。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-22 カメラとステージの当たり判定(案A: 線分だけ)
- `CameraManager::FixStageCollision(pos, target)`: 注視点→カメラに `MV1CollCheck_Line`(ステージモデル)。当たったら当たった点から `kCollisionMargin`(20)手前に寄せる(最短 `kCollisionMinDist` 30)。
  - 寄るときはすぐ、離れるときは `kCollisionReturnRate`(0.1/F)で戻る。補正後の距離は `m_collisionDist`(-1=補正なし)。
  - Stateの `m_pos` は書き換えず、描画用の `m_renderPos` だけ補正。シェイクは補正の後に足す。
- 対象は PlayerCamera / LockOnCamera / BattleSmoothCamera(`CameraManager::IsStageCollisionCamera()`、今のステートの種類を見る)。演出カメラ・フォトモードは判定しない。
  - 対象のカメラ→演出カメラに切り替わった瞬間は補正が消えるので、壁ぎわだと一瞬飛ぶかも。
- `CameraManager::Init` の stage 引数(今まで使っていなかった)を `m_stage` に入れるようにした。
- 線分なので太さが無い(壁ぎわでニアクリップが壁にめり込む・細い隙間を抜ける)。気になったら案B(球の二分探索)にする。設計の案は会話の中だけ。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-22 攻撃・演出中に縁から落ちないようにする(落下禁止)
- `Collider::SetPreventFall(bool)` / `m_preventFall`。立っている間は、`CollisionManager::PreventFall()` が毎フレーム縁で水平速度を止める。
  - 場所: `Update()` の押し戻しループの後、`SnapToGround()` の前。
  - `HasFloorAt()`: 移動先の足元から真下に `MV1CollCheck_Line`(長さは SnapToGround と同じ)。床(法線 y>=0.5)が無ければ false。
  - X→Z の順に1軸ずつ試すので、縁では辺に沿って滑り、角では止まる。空中(`m_wasFloor` が false)は対象外。
- オンにしているステート(Enter で true、Exit で false): PlayerStateAttack / DashAttack / SkillAttack / GuardBreakAttack / Assasin / PartBrokenKill / Dodge / JustDodge。
  必殺技(Ult)・被弾・弾かれは対象外(落ちる)。
- 敵: EnemyPatrol / Chase / Caution / Back / Attack。止まっているステート(Idle / AttackPre / AttackEnd)と被弾系は対象外。
- 足の中心で判定しているので、半分くらいはみ出して止まる。気になったら `moveVec` に進む向きへ半径分を足す。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-22 BattleSmoothCamera から戻る条件を追加
- 問題: カメラの向きが毎フレーム「敵→プレイヤー」にくっついていて、移動はカメラ基準なので、歩くとカメラが回って移動が引っ張られる。空中の敵が真上近くにいると向きが暴れる。
- `BattleSmoothCamera::IsExitCondition()`: 右スティックに加えて、次のどれかで PlayerFollowCamera に戻る(定数は .cpp 先頭)。
  - Move / FastRun が `kExitMoveFrame`(30F、実フレーム)続いた(`Player::IsMoveState()` を追加)。Idle で止まっているだけなら戻らない
  - 敵との水平距離が `kExitDistance`(800)より遠い
  - 敵が空中で、水平距離が `kExitNearDistance`(100)より近い
- また攻撃が当たれば `OnPlayerAttackHit` で入り直す。戻るときがカクつくなら `GetOverrideBlendSetting` を足す。
- 案2(向きの更新を緩める)・案3(スティックを倒している間は入力の基準の向きを固定)はやっていない。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-24 ボスの背中の腕(BossArm) Step1: 背中に付けて描くだけ
- ボスの設計(会話で決めた方針): 妖怪の四本腕。上の2本=本体の腕、**背中の2本=別モデルの `BossArm`(EnemyBase 継承)**。
  腕を EnemyBase にするのは、ガードブレイク→打ち上げ→空中コンボ・ロックオン・吸い寄せ・被弾処理を敵と同じに使うため。
  打ち上げは「腕の RigidBody=手の位置として物理で飛ばし、肩から腕の長さで制限、モデルは手の方を向ける」予定(Step2)。
- `Character/Enemy/BossArm.h/.cpp` を追加し、vcxproj/filters に登録。左右2本(`BossArm::Side`)。
  - モデルは仮でガンナーの盾なしの左腕(`EnemyGunnerPartLeftArmModel`、右も同じモデル)。骨・アニメ無し。当たり判定・ステートもまだ無い(CollisionManager に登録されない)。
  - 位置: Draw で「ボスの体の向き(MV1GetMatrix の回転だけ)＋付け根フレームの位置」×ずらし。ずらしの軸はボスのモデルの軸。背骨の曲がりには追従しない。
  - 付け根のフレームは `spine_03 → spine_05 → spine_02 → spine_01` の順に探し、無ければ0(BossPirate.mv1 のフレーム名はまだ確認していない)。
  - ボスが消えたら `m_isVanished` を立てて EnemyManager に消してもらう。ボスの `SetOpacity` で腕も消える。
- `EnemyBase::CreateChildEnemies()`(標準は空)。ボスが上書きして腕を作って Init し、weak_ptr を持つ。
  `EnemyManager::StartPhase` でボスの直後に `m_enemies` に追加(ボスの後に更新される)。`m_phaseEnemies` には入れない。
- `EnemyBase::IsTargetable()`(標準 true、腕は false)。`LockOnManager::CollectEnemiesRange`・`PlayerStateAttack` の2か所・`PlayerStateAttackBase` で見る。
- **調整**: ボスの `DebugDraw`(F3 表示中＋F2 の ImGui 表示中)で「Boss Arm Editor」。腕ごとに Root Frame(番号とフレーム名)・Pos・Rot・Scale。
  「Mirror Left -> Right」で左の値を左右反転して右へ。「Copy Constants」でクリップボードに定数の形で出るので、`BossArm.cpp` 先頭の定数(左腕の値)に貼る。右腕はコンストラクタで左を反転して作る。
  F3 中は付け根に緑の球と「BossArm L/R」。
- (追記) 右腕は左腕のモデルを `CalcArmMatrix` のスケールXをマイナスにして鏡写しにする(ずらしの反転だけでは形が左腕のままだった)。
  面の向きが逆になるので、右腕だけ全メッシュを `DX_CULLING_RIGHT` にしている。本物の左右の腕モデルができたらこの反転は外す。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**(F4 でボスの手前へ)。

### 2026-10-24 ボスの腕のモデル(ボスから切り出し)とアニメ
- **モデル**: `data/Enemy/BossPirate/BossArmL.mv1` / `BossArmR.mv1`(`AsyncData::BossArmLModel / BossArmRModel`)。ボスの mv1 から clavicle_l / clavicle_r 以下の腕だけを切り出したもの。
  - 作り方: `tools/BossArmExtract/`(extract.cpp・verify.cpp・build.bat。DxLib のコンソールなしツール)。`extract.exe ボス.mv1 出力フォルダ l|r 0.5`。
    各骨を少し平行移動して参照メッシュの頂点がどれだけついてくるかでウェイトを求め、clavicle 以下のウェイトが3頂点とも0.5以上の三角形だけ残す。
    骨の階層・行列・テクスチャ(`Texture/〜.png`)・アルファテストはボスと同じにして .x に書き、DxLib で読み直して mv1 に保存。.x は保存していない(ツールで作り直せる)。
  - **フレームはボスの87個全部を同じ順番で持つ**(メッシュは腕だけ。Pirate_Body フレームに入れている)。
    骨の名前だけ合わせた腕のモデルに `MV1AttachAnim(腕, 番号, ボス, TRUE)`(名前で当てる)をしても動かなかったため、フレーム番号で当てる(`FALSE`)ようにした。
  - Pirate.FBX(Unity の Assets/Pirate/Mesh)は DxLib で読めなかった(-1)。Blender(Steam 版 5.2.3)は使っていない。
- **Animation**: `SetAnimSourceModel(handle)` を追加(-1 なら今まで通り自分のモデル)。設定すると、アニメ番号・総フレームはそのモデルから取り、
  アタッチは `MV1AttachAnim(model, no, src, FALSE)`(`AttachAnim()`)。プレイヤー・普通の敵は変化なし。
- **BossArm** をこのモデルに差し替え(仮のガンナーの腕・右腕の反転・カリング変更は削除)。
  - 描画: モデル全体はボスと同じ行列、腕のモデルの `spine_03` だけ `MV1SetFrameUserLocalWorldMatrix` で「ボスの付け根フレーム(spine_03)＋ずらし」に上書き。鎖骨から先はアニメで動く。背骨の曲がりにもついてくる。
  - ずらしは付け根を中心に、ボスの体の軸で 大きさ→回転→移動。**全部0・倍率1でボスの腕にぴったり重なる**(定数をこの値に戻した。前の仮モデル用の値は使えない)。
  - アニメ名はボスのアニメCSV。`EnemyIdle` に入れ、`m_status.idleTime = FLT_MAX` と `NextAfterIdle / NextAfterRecover`→Idle で歩き出さない。
  - ImGui の Boss Arm Editor に Anim(ボスのアニメ一覧)・Loop・Play・Idle(Idle に戻す)・今のフレームを追加。F3 中の文字にステート名。
- MSBuild(Debug x64)でビルド・リンクは通った。ツールの画像で、ボスと同じアニメを当てた腕がボスの腕にぴったり重なること、背骨の上書きで腕がついてくることは確認。**ゲームでの確認はまだ**。
- (追記) 腕のアニメ名の割り当てを `data/Enemy/BossArmAnim.csv` に分けた(最初はボスの `BossEnemyPirateAnim.csv` のコピー)。
  `DataManager::GetBossArmAnimData()`、BossArm のコンストラクタで使う。**animName はボスのモデル(BossPirate.mv1)にあるアニメ名**にすること(腕はボスのアニメを借りるため)。
  敵のステートが使う key(Idle / Hit / BlockLoop など)は消さないこと。起動時に1回読むだけ(Q では読み直さない)。

### 2026-10-24 一時的な上からのライト(TempTopLight)
- ゲーム内が暗くて見づらいので、真上から照らすディレクショナルライトを足した。`Render/TempTopLight.h/.cpp`(vcxproj/filters に登録)。
  - SceneMain のメンバー `m_tempTopLight`、`SceneMain::Init` の最初のライト作成の直後で `Init()`。解放はデストラクタ。
  - 値は .cpp 先頭: 向き(0,-1,0)、ディフューズ0.6、スペキュラ0、アンビエント0.25。
- **オフ**: `TempTopLight.h` の `#define TEMP_TOP_LIGHT_ENABLE 1` を 0。**完全に消す**: ファイル2つを削除して `[TempTopLight]` で検索した行を消す(SceneMain.h 2行・SceneMain.cpp 1行)＋ vcxproj/filters。
- MSBuild(Debug x64)でビルドは通った。ゲームでの明るさの確認はまだ。

### 2026-10-24 第一ラウンドのボス(BossEnemyPirateR0)
- 方針: ボス戦は2ラウンド。**第一ラウンド = `BossEnemyPirateR0`(腕なしの海賊剣士)**、**第二ラウンド = `BossEnemyPirate`(背中の腕 BossArm あり)**。
  切り替えはフェーズで行う予定(R0 が AllDead → 変身の演出イベント → 第二ラウンドを出す)。
- `Character/Enemy/BossEnemyPirateR0.h/.cpp`: BossEnemyPirate のコピーから、背中の腕(CreateChildEnemies・Boss Arm Editor の DebugDraw・SetOpacity の腕・メンバー)を外したもの。vcxproj/filters に登録。
  - ステータス(EnemyStatus.csv の `BossEnemyPirate` 行)・攻撃CSV・アニメCSV・Q の読み直しは**第二ラウンドと共有**。分けたくなったら行・ファイルを足して差し替える。
- `EnemyType::BossEnemyPirateR0` / 配置CSVの type `BossEnemyPirateR0`(`StringToEnemyType`、`EnemyManager::CreateEnemy`)。Unity の EnemySpawnType にも足すこと。
- F4(`PhaseManager::DebugSkipToBossPrevPhase`)は、BossEnemyPirate / R0 のうちフェーズ番号が小さいほうの手前へ飛ぶようにした。
- MSBuild(Debug x64)でビルドは通った。
- (追記) 配置: `Enemies003.csv` でフェーズ7に `BossEnemyPirateR0`、フェーズ8に `BossEnemyPirate`(どちらも 0,-1170,31500・180度・Idle・group Boss)。
  遷移 `PhaseStage003.csv`: `7 AllDead→8 / None`、`8 AllDead→-1 / GameClear`。**Unity から配置を書き出し直すと消える**。
  - R0 の死体が同じ場所に残ったまま第二ラウンドが出る(死体は上限5体まで残る)。変身の演出を作るときに、R0 を消す(StartVanish)か隠す処理を入れること。ゲームでの確認はまだ。

### 2026-10-24 Character/Enemy を敵ごとのフォルダに整理
- **これより前のログに出てくる敵のファイルのパスは整理前のもの**。今の構成(「敵の名前/」に本体、「敵の名前/Part/」に部位・武器):
  - `Character/Enemy/` 直下: EnemyBase, EnemyManager, EnemyPart(全敵共通)と `State/`(変更なし)
  - `Swordman/` EnemySwordman
  - `Gunner/` EnemyGunner、`Gunner/Part/` EnemyGunnerWeapon
  - `Bazooka/` EnemyBazooka、`Bazooka/Part/` EnemyBazookaWeapon
  - `BossPirate/` BossEnemyPirate(第二ラウンド), BossEnemyPirateR0(第一ラウンド)、`BossPirate/Part/` BossArm
- include はプロジェクト基準に直した(例 `"Character/Enemy/BossPirate/Part/BossArm.h"`、`"Character/Enemy/EnemyBase.h"`)。同じフォルダはファイル名だけ。
  移したファイルの `../../Math/〜` `../../DataLoader/〜` も `Math/〜` `DataLoader/〜` にした。EnemyManager.cpp・SceneMain.cpp の include も変更。
- vcxproj/filters のパスを変更(filters のフィルタ名は今まで通り「ソース ファイル」「ヘッダー ファイル」)。MSBuild(Debug x64)でビルドは通った。
- **新しい敵を足すときも、このフォルダ分けに合わせる**。

### 2026-10-24 海賊のボスに剣(カトラス)を持たせた
- モデル: `data/Enemy/BossPirate/PirateSword.mv1`(`AsyncData::BossPirateSwordModel`)。元は Unity の `Assets/Pirate/Mesh/Pirate_Sword.FBX`。
  - Blender(Steam 版)で .x に書き出し(`tools/PirateSword/sword2x.py`。ボスの mv1 と同じ軸 x=x, y=Blenderのz, z=Blenderのy、×100 で cm。原点は FBX のまま=握り)、DxLib で読み直して mv1 に保存(`tools/PirateSword/sword.cpp`)。全長約106、刃は +Y。
  - テクスチャは **`Texture/PirateSword_Albedo.png`(剣専用、アルファなしの RGB)**。`Pirate_Details_Albedo.png` はアルファが全部0(Unity の滑らかさ)で、.x から読むと半透明扱いになって剣が見えなくなったため、RGB だけ写したコピーを作った。
- `Character/Enemy/BossPirate/Part/BossPirateWeapon.h/.cpp`(EnemyGunnerWeapon と同じ作り)。持ち主は**モデルのハンドル**で受け取る(第一・第二ラウンドの両方で使うため)。vcxproj/filters に登録。
  - 右手 `hand_r` の行列 × ずらし。値は .cpp 先頭。Tポーズで「刃が前、護拳が指の側、握りが手のひら」になるようにツールで計算した(オフセット -6.12,-3.64,0.05 / 回転 -99.73,4.00,-92.40)。Idle・攻撃・ガードの画像で確認済み。
- BossEnemyPirate / BossEnemyPirateR0 の両方: Init の最後で生成、Update の最後(アニメ・首の後)で `Update`、Draw で描画(血殺中は黒)、SetOpacity で一緒に消える。
- ボスのモデルには元から腰の剣(フレーム `Pirate_Sword_In_Belt`)があるので、今は腰と手に1本ずつ見える。消すなら `MV1SetFrameVisible`。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-24 敵のアニメCSVに animSpeed(再生速度)
- 敵のアニメCSV 5つ(EnemySwordman / EnemyGunner / EnemyBazooka / BossEnemyPirate / BossArm の `〜Anim.csv`)を `key,animName,animSpeed` にした。最初は全部 1.0。
  - 3列目は省略・空欄なら 1.0(プレイヤーのアニメCSVは2列のままで変化なし)。数字以外を書くと `std::stof` の例外で止まる。
- (変更後の作り)Animation で自動で掛けるのはやめて、**プレイヤーと同じく ChangeAnim の引数で速度を渡す**形にした。
  - 読み込み: `DataManager::LoadAnimData` で `AnimData::animSpeeds`(**キーは key**。GetAnimName と同じ)に入れる。
  - `CharacterBase::m_animSpeeds` / `GetAnimSpeed(key)`(無ければ1.0)。各敵のコンストラクタ(`m_animNames` の直後)と BossArm、ボスの I キーの読み直しで `m_animSpeeds` を入れる。
  - 各ステートの ChangeAnim の timescale に `owner->GetAnimSpeed("Idle")` などを渡す。**もともと速度を書いていた所は掛け算**(例 `0.5f * owner->GetAnimSpeed("BlockLoop")`)なので、CSV が1.0なら今まで通り。
    対象: Idle / Patrol / Chase / Caution / Back / HitAir / HitGround / HitDrop / Die / Guard / GuardBreak / Parry、各敵の Init(Idle)。
  - **使っていない所**: EnemyAttack / EnemyAttackPre(攻撃CSVの再生速度を使う)、EnemyAssasined / AirAssasined / PartBrokenKilled(プレイヤーのアニメと速さを合わせている)、BossArm の ImGui の Play。
  - 新しくステートを作ってアニメを流すときも `owner->GetAnimSpeed(key)` を渡すこと。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。
- (追記) **I キーでボスのアニメCSV(`BossEnemyPirateAnim.csv`)を読み直す**。BossEnemyPirate / R0 の Update(Q の読み直しの直後)。
  `DataManager::ReloadBossEnemyPirateAnimData()`(空にしてから読む)→ `m_animNames` と速度の表を入れ直す。今流れているアニメの速度は変わらず、次にアニメが切り替わったときから効く。
  - **I は EnemyManager の「敵をずっと Idle にする」切り替えと同じキー**なので、押すと両方が起きる。背中の腕(BossArmAnim.csv)は読み直さない。

### 2026-10-24 BossPirate.mv1 の更新(アニメ追加)に合わせた作業
- ユーザーが BossPirate.mv1 を新しくした(アニメ 45→60)。フレーム87個の名前・並び・行列、メッシュ、マテリアルは前と同じ(ツールで比較済み)
  → **背中の腕(BossArmL/R)・剣はそのまま使える**(作り直し不要)。
- **テクスチャ**: 新しい mv1 にはテクスチャが無かったので、前の mv1(git の HEAD)から、マテリアルごとにテクスチャ・アルファテスト・色を写して上書き保存した。
  ツールは `tools/BossPirateRetex/retex.cpp`(作業フォルダを `data/Enemy/BossPirate` にして `retex.exe 前のmv1 ログ` で実行)。31個とも前と同じ割り当て(髪/服/体/小物)になったのを確認、画像でも確認。
  - `MV1AddTexture` の戻り値はテクスチャの番号ではない(成功で0)。番号は足した後の `MV1GetTextureNum - 1`。
  - **次に mv1 を更新したときも、マテリアルの並びが同じならこのツールで貼り直せる**。
- **アニメCSV**: 増えた15個を `BossEnemyPirateAnim.csv` と `BossArmAnim.csv` の最後に追加(animSpeed は 1.0)。
  key: `DodgeB/F/L/R`(Dodge_Combat_〜)、`Combo02_01` `Combo02_04` `Combo03_01〜04` `Combo04_02〜04`(Combo_Attack_〜)、`ComboWave05_03`、`AttackUp01`(Attack_Up_01)。
  CSV のアニメ名がすべて mv1 にあること、mv1 のアニメがすべて CSV にあることを確認した。まだどの攻撃CSV・ステートからも使っていない。

### 2026-10-24 敵の攻撃の終わり(attackFinishFrame が空欄のとき)を修正
- `EnemyAttack::Update`: `attackFinishFrame` が負(CSV で空欄 = -1)のときは `m_anim.GetAnimEndFlag()`(animEndFrame を考えたアニメの終わり)で終える。
  前は `今のフレーム >= -1` で比べていたので、空欄の攻撃は始まった瞬間に終わっていた。数字が入っている攻撃は今まで通り。
- MSBuild(Debug x64)でビルドは通った。

### 2026-10-24 敵の連続攻撃(AttackEnd の後に nextAttack でつなぐ)
- 流れ: AttackPre → Attack → AttackEnd →(nextAttack があれば)Attack → AttackEnd → … → Back。攻撃の流れのステートの間なので攻撃権は最後まで持ったまま。
- `EnemyBase::NextAfterAttackEnd`: 今の攻撃の `nextAttack` が0以上なら `m_currentAttackIndex` をそれにして `EnemyAttack(owner, true)`、無ければ今まで通り `EnemyBack`。行数を超える番号は assert。
- `EnemyAttack` のコンストラクタに `isChain`(初期値 false。AttackPre から来る1撃目は false)。`Enter` の最初(アニメ・判定を作る前)で、isChain なら踏み込み先を決め直す:
  - `IsChainAttackLookAtPlayer()` が true: `m_targetPos` = プレイヤーの位置、`ToPlayerLook()`。判定の位置・踏み込みはすぐプレイヤーの方、モデルは数フレームかけて回る。
  - false(普通の敵): 向きはそのまま、`m_targetPos` = 今の位置 + 向き × 1000(`kChainForwardDistance`)。1撃目で通り過ぎた踏み込み先へ戻らないように。
- `EnemyBase::IsChainAttackLookAtPlayer()`(標準 false)。BossEnemyPirate / BossEnemyPirateR0 で true。
- F3 の Attack の情報に `#攻撃の番号` と連撃なら「連」。
- **CSV の書き方**: 1撃目の行の `nextAttack` に2撃目の番号、`attackEndTimeFrame` を短く(5〜15F)、`attackFinishFrame` を振り終わりのフレームに(AttackEnd の間もアニメの残りが流れる)。
  2撃目の行は `attackRange` を0(単体では選ばれない)、`nextAttack` を-1。
- AttackEnd 中は「弱攻撃でもガードしない」ままなので、連撃の間に攻撃を当てられると被弾して連撃が止まる。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**(CSV にまだ連撃の行が無い)。
- (追記) ボスの攻撃の分岐用に、BossEnemyPirate / BossEnemyPirateR0 に `SelectAttackIndex()` の override(protected)を足した。中身は **ユーザーが実装する**。今は `EnemyBase::SelectAttackIndex()` を呼ぶだけ(TODO コメントあり)。
  - 呼ばれるのは `NextAfterIdle` / `NextOnArrived`(攻撃を選べたら攻撃権を頼んで AttackPre へ)と、Chase の「出せる攻撃があるか」の確認。-1 を返すと攻撃しない。
  - 敵共通の版がしていること: クールタイム中なら-1、`attackRange <= 0`(連撃の2撃目など)は選ばない、プレイヤーとの水平距離が attackRange 以内の攻撃からランダム。

### 2026-10-24 ImGui を docking 版にして、ウィンドウの外に出せるようにした
- `imgui/` を公式の `v1.92.9-docking` に入れ替えた(前の master 版 1.92.9 は公式から手を加えていなかったので、ファイルを上書きしただけ)。
- `imguiApp::Init`: `ImGuiConfigFlags_DockingEnable` / `ViewportsEnable`、角を丸めない・背景を不透明に。
  メイリオ(`C:\Windows\Fonts\meiryo.ttc`、無ければ標準フォント)を読むので日本語も出る。
- `imguiApp::Render`: `UpdatePlatformWindows` / `RenderPlatformWindowsDefault` の前後で DxLib の描画先(RTV/DSV・ビューポート)を覚えて戻す。
- `Application::Init` で `SetAlwaysRunFlag(TRUE)`(外のウィンドウを触るとゲームが非アクティブになって止まるため)。
- **日本語の文字列は `imguiApp::ToUtf8()` を通して出す**(プロジェクトは MultiByte・`/utf-8` なしなので、ソースの日本語も CSV も Shift-JIS)。
- ImGui のウィンドウを外に出すとゲームのウィンドウが非アクティブになるので、その間はキー入力(CheckHitKey)がゲームに入らないかも。

### 2026-10-24 ボスのデバッグウィンドウ(Enemy Debug)
- `Character/Enemy/EnemyBaseDebugWindow.cpp`(EnemyBase のメンバー。EnemyBase.cpp が長いので分けた)。vcxproj/filters に登録。
- `EnemyBase::HasDebugWindow()`(標準 false)。BossEnemyPirate / R0 で true。`EnemyManager::DrawDebugWindows()` を SceneMain の `#ifdef _DEBUG` で、**F2(ImGui 表示)中だけ**呼ぶ(F3 とは関係なし)。
- 中身:
  - Status: ステート名＋情報、HPバー、位置、プレイヤーとの距離(最大の攻撃距離)、攻撃のCT、最後の攻撃、確定ヒット、ガードゲージ、攻撃権・発見・接地・死亡など。
  - Control: **Stop AI**(この敵だけ Idle から動かない。移動系のステートは Idle に戻す。攻撃はウィンドウから出せる)、Lock HP、Reset HP / CT、ステートを直接変える(Idle/Chase/Caution/Back/Guard)。
  - Attack: 攻撃CSVの一覧(クリックで選ぶ・**ダブルクリックか Execute で出す**)。Skip Pre で予備動作を飛ばす。Loop で Idle に戻ってから間隔をあけて同じ攻撃を繰り返す(Stop AI と一緒に使う)。
    出している攻撃は一覧で赤。攻撃権・クールタイムは見ずに出す。連撃(nextAttack)はそのままつながる。
  - 選んだ攻撃の詳細・Status Edit: その場で値を変えられる(**Q で CSV を読み直すと元に戻る**)。
- `SetDebugIdle` を変えた: `m_isDebugIdle` = I キー(全体)|| Stop AI。攻撃中を Idle に戻すのは I キーのときだけ。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**(外に出したウィンドウの描画・入力、デバッグウィンドウの動き)。

### 2026-10-24 大きくのけぞる被弾ステート(EnemyHitLargeHit)
- `Character/Enemy/State/Hit/EnemyHitLargeHit.h/.cpp`: EnemyHitGround のコピー。違いはアニメ(`HitLargeF` = Hit_Large_Combat_F)と GetName("HitLargeHit")だけ。
  吹き飛ばし・プレイヤーの正面に寄せる処理・終わったら NextAfterRecover(空中なら AirStay)も同じ。コンストラクタは `(owner, HitInfo)`。
  `HitLargeF` は5つの敵のアニメCSV全部にある。向きで B/L/R を使い分けたいなら Enter で選ぶ。
- (追記) `EnemyHitGround` の被弾アニメを `Hit` から **`HitF` / `HitL` / `HitR` のランダム**(`GetRand(2)`)にした。3つとも5つの敵のアニメCSV全部にある。`Hit` の key は EnemyHitAir・確殺(EnemyPartBrokenKilled)でまだ使っている。
- EnemyBase.h に include と friend、vcxproj/filters に登録。**まだどこからも入らない**(OnDamage などで `std::make_shared<EnemyHitLargeHit>(GetWeakPtr(), hitinfo)` に遷移させる)。
- MSBuild(Debug x64)でビルドは通った。

### 2026-10-24 敵の攻撃の踏み込みでガタガタする問題
- 原因(推定): `EnemyAttack::AttackMove` が毎フレーム「`m_targetPos` - 今の位置」で向きを計算し直していたので、速い踏み込み(突進)で目標を通り過ぎると、次のフレームで逆向きになり前後に揺れていた。
  押し戻しの設定(2人で0.5ずつ、1フレーム3回)は変えていない。
- `EnemyAttack::m_moveDir`: 踏み込む向きを `Enter` で1回だけ決めて固定(連撃の向き直しの後)。目標が自分の位置と同じなら今の向き。
- 目標を通り過ぎたら止まる: 1フレームの移動量を「目標までの残りの距離(進む向きの成分)」までに抑える。通り過ぎていたら0。判定のON/OFFはそのまま。
  - 目標はプレイヤーの位置(予備動作の途中まで追う)なので、プレイヤーがそこにいればぶつかって押し戻しで止まる。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。プレイヤーに当たっている間の揺れ(押し合い)が残っていれば、別の対策が要る。

### 2026-10-25 第一ラウンドのボスは攻撃中に怯まない
- `BossEnemyPirateR0::OnDamage`: `isAttacking = IsAttackFlowState(m_currentState)`(AttackPre/Attack/AttackEnd)なら、ダメージ・発見・血しぶきだけ出して return(ガード・ヒットステートに行かない)。HP0ならその場で `EnemyDie`。
  - 確定ヒットの回数・吸い寄せは攻撃中は処理しない。スタンを入れるときは isAttacking の条件に足す。第二ラウンド(BossEnemyPirate)にはまだ入れていない。
- MSBuild(Debug x64)でエラーなし。ゲームでの確認はまだ。

### 2026-10-25 第一ラウンドのボスの技の選び方(状況ルール)
- `BossEnemyPirateR0::SelectAttackIndex`: クールタイム中は-1。その後、上から順に当てはまったらその技(定数は .cpp 先頭)。
  1. 前の全体攻撃から `kLargeAttackInterval`(3)回攻撃した＋全体攻撃(1)の attackRange 以内 → 全体攻撃
  2. 背後(正面との内積 < `kBackDot` -0.5)＋薙ぎ払い(2)の attackRange 以内 → 薙ぎ払い。正面は `m_targetVec`
  3. 距離が `kRushMinDistance`(400)〜`kRushMaxDistance`(1000) → 突進(6)。CSV の attackRange は見ない
  4. どれでもなければ、横斬り(0)かコンボの1撃目(3)を1/2ずつ(attackRange 以内のものだけ)
  - (ユーザーが kLargeAttackInterval を10に変えた)
- 回数は `NextAfterAttackEnd` の override で数える(連撃は最後まで終わって1回)。`m_attackCountSinceLarge`。
- Chase は「SelectAttackIndex が攻撃を返したら攻撃」なので、400〜1000 の距離だと近づかずに突進する。200〜400 は近づく。
- MSBuild(Debug x64)でエラーなし。ゲームでの確認はまだ。第二ラウンドは EnemyBase の選び方のまま。

### 2026-10-26 視野角のオフセット(戻すまでずっと足す)
- `CameraManager::SetFovOffset(angleDeg, blendFrame)` / `ResetFovOffset(blendFrame)` / `FovOffsetUpdate()`。設計は `../進捗/note_カメラの視野角.md` の9章。
  - 今の値から目標へ `blendFrame` かけて EaseOut(2.0)。途中で変えても今の値から続く。**実フレーム**で進む。最後に設定した人が勝ち。
  - `Update`: `m_renderFov = GetFov() + FovOffsetUpdate() + FovPunchUpdate()` を `kFovMin/Max`(10〜120°)で clamp。
  - 演出カメラ中は足さない: `IsFovOffsetCamera()`(PlayerCamera / LockOn / BattleSmooth だけ true)。効き具合 `m_fovOffsetWeight` を `kFovOffsetWeightFrame`(10F)で 0↔1 にするので、出入りで画面が飛ばない。パンチは今まで通りどのカメラでも足す。
    (`IsStageCollisionCamera` は今は全部 true を返すようになっているので、別の関数にした)
- 使っている所: `PhaseManager::ChangePhase` の `BossAppear` イベント(6→7、Area_Boss に入ったとき)で `SetFovOffset(20, 60)`(`kBossFovOffset` / `kBossFovBlendFrame`)。
  戻す処理は無い(リザルトでシーンごと作り直すので)。F4 で飛んでもエリアに入れば広がる。BossAppearCamera 中は効かず、通常のカメラに戻ると10Fで出てくる。
- MSBuild(Debug x64)でエラーなし。ゲームでの確認はまだ。

### 2026-10-26 使っていないカメラステートを削除
- `TitleCameraState` / `ResultCameraState` / `UltStartCamera` / `FinishingFirstCamera` のファイルと、`CameraStateName`・`CameraStateBase::Type`・`ChangeStateFromScene` の case・vcxproj/filters から削除。
- KessatsuUI のラストヒット用の配置(`IsLastHitEvent` / `DrawLastHit`)も削除。いつも必殺技の配置(`DrawUlt`)で出す。CameraManager も受け取らなくなった(`UIManager::InGameInit` の引数は残してある)。
- 他にも使っていない: `FinishingSecondCamera`、`BossAppearCamera` / `Stage1FazeCamera` / `Stage2FazeCamera`(どこからも生成されていない)。
- MSBuild(Debug x64)でビルドは通った。

### 2026-10-26 第二ラウンドのボスに第一ラウンドの中身を移植
- `BossEnemyPirate.h/.cpp` を `BossEnemyPirateR0` の中身で置き換え、背中の腕(BossArm・CreateChildEnemies・Boss Arm Editor の DebugDraw・SetOpacity の腕・メンバー)だけ残した。
  - 入ったもの: 技の選び方(`SelectAttackIndex`・`NextAfterAttackEnd` の回数、定数は .cpp 先頭)、攻撃中は怯まない `OnDamage`、被弾中にクールタイムが明けたら攻撃へ(Update)、被弾は `HitLargeHit` / `HitGround` だけ(空中・叩きつけに行かない)。
  - 定数は R0 と別々に持っている(どちらかを変えてももう片方は変わらない)。
- 移植前のファイルは scratchpad にだけ残してある(git の HEAD にも前の版がある)。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-26 第二ラウンドのボスを BossEnemyPirateR1 に改名・CSVを分けた
- `BossEnemyPirate.h/.cpp` → `BossEnemyPirateR1.h/.cpp`(クラス名も)。`EnemyType::BossEnemyPirateR1` / 配置の type `BossEnemyPirateR1`(`Enemies003.csv` のフェーズ8の行も変更)。
  **Unity の EnemySpawnType も `BossEnemyPirate` → `BossEnemyPirateR1` に直すこと**(古い名前のままだと書き出した行が読み込みで飛ばされる)。
- 第二ラウンド用の CSV(最初は第一ラウンドのコピー):
  - `data/Enemy/BossEnemyPirateR1Attack.csv` / `BossEnemyPirateR1Anim.csv`。`DataManager::GetBossEnemyPirateR1AttackRawData / GetBossEnemyPirateR1AnimData`、Q / I の読み直しもこちら。
  - `EnemyStatus.csv` の `BossEnemyPirateR1` 行(R1 の `kStatusType`)。
- 第一ラウンド(R0)は今まで通り `BossEnemyPirateAttack.csv` / `BossEnemyPirateAnim.csv` / ステータスの `BossEnemyPirate` 行を使う。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。
- 注意: Git Bash の `sed -i` は CRLF を LF にしてしまう。ソースを sed で書き換えたら `unix2dos` で戻すこと。

### 2026-10-26 全モデルのマテリアルの色を変更(拡散・反射0.1、環境0.9)
- System.cpp で読んでいる mv1 16個(プレイヤー・刀2本、敵・ガンナー・銃・RPG7・部位、ボス・背中の腕・剣、ステージ2つ)の全マテリアルを
  ディフューズ / スペキュラの RGB = 0.1、アンビエントの RGB = 0.9 にして上書き保存した(アルファはそのまま)。
- ツール: `tools/ModelMaterialColor/`(`matcolor.exe ログ mv1 mv1 ...`、作業フォルダは 2026_winter/2026_winter。値は .cpp 先頭)。保存は圧縮なし(法線・位置・アニメを縮めない)なのでファイルが2〜4割大きくなった。
  マテリアル数・アニメ数・テクスチャ数が保存前と同じことは確認した。
- 元の色はバラバラだった(ボス dif1.0/amb0.2、プレイヤー dif1.0/amb0、Enemy・部位・TestStage は dif0 など)。`red_katana_ult` の赤いマテリアル(dif 0.8,0,0)も灰色になった。元に戻すなら git の HEAD から。
- アンビエントはライトのアンビエントと掛け算になる。ゲームでの明るさの確認はまだ。
- (追記) 武器5つ(katana_blend / red_katana_ult / AK74 / RPG7 / PirateSword)は元の色に戻した(git の HEAD と同じ)。色を変えたのは体・ステージの11個だけ。
- (追記) 一時ライト `TempTopLight` を一度削除したが、やっぱり要るので元に戻した(git の HEAD と同じ)。今も上から照らしている。

### 2026-10-26 ライトの見直し(全体が白っぽいのを直す)
- 原因: DxLibの標準ライト(拡散1.0・反射1.0・環境0.33)が有効のままで、カメラ方向(1.0)・SceneMain(0.4)・TempTopLight(0.6)と重なり、拡散光の合計が約3倍になっていた。
- 変更: `SceneMain::Init` で `SetLightEnable(FALSE)`(標準ライトを切る。デストラクタで TRUE に戻すのは前から)。
  カメラのライト 拡散1.0→0.6・反射0.5→0.3(`Camera.cpp`)。TempTopLight 拡散0.6→0.5、環境0.25→0.58(切った標準ライトの環境光0.33の分を足した)。
- キャラ・ステージのモデルはマテリアルが「拡散0.1・環境0.9」なので、環境光の合計を保てば明るさはほぼ変わらない(プレビューで確認)。拡散光が効く武器や列車ステージは暗め・くっきりになる。
- **TempTopLight を消すときは環境光(0.58)の分をどこかに移すこと**(消すとキャラが暗くなる)。ゲームでの確認はまだ。

### 2026-10-26 第二ラウンドのボスがガードしたら背中の腕もガードする
- 方針(会話で決めた): キャラは `EnemyBase → BossBase(予定、R0/R1 の共通部分) → R0 / R1`、腕は EnemyBase のまま。
  ステートは `EnemyStateBase` をそのまま使い、ボス専用の動きは**共通のステートを継承したボス用のステート**で書く(EnemyBase に OnStateChanged のような通知は足さない)。腕は言われたアニメを流すだけ。
- `Character/Enemy/BossPirate/State/BossGuard.h/.cpp`(`EnemyGuard` 継承)。vcxproj/filters に登録。
  Enter で親の Enter の後に腕へ `BlockLoop`(0.5倍、ボスと同じ)、Exit で親の Exit の後に腕を Idle に戻す。
- `BossArm::PlayAnim(key, isLoop, timeScale)`(BossArmAnim.csv の key、速度は CSV の animSpeed を掛ける) / `ReturnToIdle()`(EnemyIdle へ)。
- `BossEnemyPirateR1::GetLeftBackArm / GetRightBackArm`(lock して返す)。`OnDamage` のガードへの移行を `BossGuard` にした。
- ガードブレイク・弾きは EnemyGuard の中で `EnemyGuardBreak` / `EnemyParry` を直接作るので、今は Exit で腕が Idle に戻るだけ。
  ブレイクで腕を切り離して打ち上げるときは、EnemyGuard に次のステートを作る virtual を足して BossGuard で上書きする予定。
- デバッグウィンドウの Guard ボタンは普通の `EnemyGuard` を作るので腕は動かない。確認は実際に攻撃してガードさせる。
- MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**(背中の腕の Block_Loop が「前に出す」形に見えるか)。

### 2026-10-26 背中の腕の切り離し Step1(ガードブレイクをボス用に差し替える入口)
- 方針(会話で決めた): 見た目は背骨の上書きを続けて、切り離した瞬間の背骨の行列＋移動・回転で描く(案1a)。動きは最初は決めた軌道だけ、あとで当たり判定・空中ステート(案2b)。
  手順: Step1 ガードブレイクで腕に知らせる → Step2 `BossArmDetached` / `BossArmReturn` で上げて戻す(見た目だけ) → Step3 当たり判定・Yの打ち上げの対象・吸い寄せ。
- `EnemyGuard::CreateGuardBreakState()`(protected virtual、標準は EnemyGuardBreak)。`OnGuardHit` のガードブレイクはこれで作る(ここまではユーザー)。
- `BossGuard::CreateGuardBreakState` で `BossGuardBreak` を返す。`Character/Enemy/BossPirate/State/BossGuardBreak.h/.cpp`(`EnemyGuardBreak` 継承、vcxproj/filters に登録)。
  Enter / Exit は親を呼ぶだけ。腕の `Detach()` / `RequestReturn()` の呼び出しは **TODO のコメント**(Step2 で BossArm に作ったら外す)。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ(F3 でボスのステート名が BossGuardBreak になるか)。

### 2026-10-26 F4 で第一ラウンドから第二ラウンドへ飛ぶ
- `PhaseManager::DebugSkipToBossPrevPhase`: 今のフェーズが R0 のいるフェーズなら、敵を消して R1 のフェーズへ `ChangePhase`(event は None、プレイヤーはそのまま)。それ以外は今まで通りボスの手前へ。
- `PhaseManager::FindEnemyPhase(type)`(その種類の敵がいる一番早いフェーズ、いなければ-1)を追加。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-26 ボスのやられ判定をカプセルに
- R0 / R1 の `m_hitCol` を球からカプセルにした。値は押し戻しの判定と同じ(`bodyColOffsetY` / `bodyColHeight` / `bodyColRadius`)。
  ステータスの `hitColRadius` / `hitColOffsetY` はボスでは使っていない(普通の敵は今まで通り球)。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-27 ToiletMech(新しいボス候補)の mv1
- 方針(会話で決めた): Sci-Fi ToiletMech を新しいボスにし、今の海賊ボスは中ボスに回す予定。腕が取れる部位破壊は見た目が変なのでやめる。
  案: ガードブレイクで膝をつかせ、プレイヤーが頭(便器)の高さまで上昇して追撃(頭は被ダメ増)。部位破壊は機械の部品を消す・機能を止める形(ジェットパック・配線など)。
- `data/Enemy/ToiletMech/ToiletMech.mv1`(Skin 3、アニメ40個)、テクスチャは `Texture/`(BaseColor の 4096 TGA を 2048 PNG に。体と便器の2枚)。まだゲームのコードからは使っていない。
  - 骨格は UE 形式(`pelvis` / `head` / `toilet_joint` など、フレーム281)。メッシュはフレームごとに分かれている(`JetpackSpine1` など。部位を `MV1SetFrameVisible` で消せる)。
  - 大きさは Tポーズで高さ約230(cm)。マテリアルは他の体のモデルと同じ 拡散・反射0.1 / 環境0.9。MetallGloss は使っていない。
  - **顔のパネル(便器の正面の黄色い顔)と体の赤・黄のライトは Emissive テクスチャ**にある。DxLib の mv1 には自発光のテクスチャが無いので、
    build_mv1 で BaseColor に Emissive を加算して焼き込んでいる。光っては見えない(ライトで暗くなる)。光らせたいなら顔の部分だけ別マテリアルにして MV1SetMaterialEmiColor か、シェーダーで。
- 作り方: Unity(`ninja_motion`)の `Assets/Editor/ToiletMechExport.cs` → `Assets/Exported/ToiletMech.fbx`(`SkinnedAnimFbxExporter` を使う。プレハブ Skin 3 の Animator Controller の全クリップ。
  バッチモードなら `Unity.exe -batchmode -projectPath ... -executeMethod ToiletMechExport.Run -quit`、エディタなら `Tools > ToiletMech > Export Skin3 FBX`)
  → DxLibModelViewer(Setting.ini の `LoadAndSave=1` にしたコピーに FBX を渡す)で `ToiletMech.mv1`
  → `tools/ToiletMech/build_mv1.exe 元のmv1 "Unityの Textures/Skin 3" 出力フォルダ` でテクスチャ・色を貼って保存(preview.png も出る)。
  - `SkinnedAnimFbxExporter` は止まったポーズだけのクリップ(Idle1_Toilet)も書き出すようにした(前は「骨が動かない」で空になっていた)。
  - エディタを開いたまま2回続けて書き出したら Unity が落ちた。続けて書き出すならバッチモードで。
- (更新)**Humanoid で書き出し直した**。4スキンのモデルと40アニメを全部 Humanoid に(ToiletMechExport の `SetupHumanoid`)。
  - ボーンの割り当てが入っていたのは Skin1 だけ(自動だと右手が見つからない)。Skin2〜4・アニメは Skin1 のアバターをコピー。アニメの移動・回転は全部ポーズに焼き込み。
  - 書き出しは **XZ だけ消す**(root・pelvis の XZ を1フレーム目に固定)。**Y は残す**(KnockStart で膝をつく、Death で倒れるなど、腰の高さが動く)。全アニメで腰の XZ の動きが0なのを確認した。
  - Humanoid にしたので動かなくなった骨: `spine_03` / `spine_05`(背骨の途中。割り当ては spine_01=Spine、spine_02=Chest)、左手の指先、`ball_l/r`、Turn系の `root`、
    TransitionIdle1ToIdle3 の `toilet_joint` と配線(便器が動くのはこのアニメだけ)。気になったら Skin1 の割り当てを Chest=spine_03 / UpperChest=spine_05 にする。
  - 前の Generic 版の mv1 は残していない(作り直すなら ToiletMechExport を Generic に戻す)。
- (更新)**頭を便器からゴーレムの兜(メカ風に塗り直し)に替えた**。`ToiletMechExport.BuildMechWithHelmet`:
  - 便器のメッシュ(マテリアルが `_Toilet_`)と、便器につながるケーブル(頂点の半分以上が `wiresToilet` の骨で動くメッシュ6個)を消す。顔のパネルも便器と一緒に無くなった。
  - `Assets/StoneGolem/Prefab/StoneGolem_Armor_Helmet` を今の形で焼いて、便器の幅×0.7(`HelmetWidthRate`)に縮め、底を便器の底に合わせて `toilet_joint` の子にする(骨で動かない固い部品。mv1 ではフレーム `Helmet`)。
    位置・回転を直すなら `HelmetOffset` / `HelmetEuler`。
  - テクスチャ `ToiletMechHelmet_Albedo.png`(`MakeHelmetTexture`): ゴーレムの兜の Albedo の明るさを暗い金属〜明るい金属に、金色の縁をメカの黄色に、Emission を黄色の線(強さ `HelmetGlowRate` 0.35)に。マテリアル名は `M_ToiletMech_Helmet`。
  - build_mv1 の4つ目の引数に兜の PNG を渡す(`Assets/Exported/ToiletMechHelmet_Albedo.png`)。便器のマテリアルが無いので便器のテクスチャは消える。
  - 高さは Tポーズで約200 になった(兜は便器より低い)。
- Unity エディタを開いたまま動かすとき: `Library/ToiletMechRun.txt` に関数名(`RunExportOnly` / `ShotAll` など)を書いてエディタを前に出すと、スクリプトの読み込み後に1回実行する。
  `ShotAll` は兜付きのメカを撮って `Library/ToiletMechShot_*.png` に出す(プレビューシーンなので開いているシーンは写らない)。

### 2026-10-27 新しいボス BossEnemyKingmech
- `Character/Enemy/Kingmech/BossEnemyKingmech.h/.cpp`: **BossEnemyPirateR0 と同じ中身**(技の選び方・攻撃中は怯まない OnDamage・被弾中にCTが明けたら攻撃・Q/I の読み直し)。vcxproj/filters に登録。
  - 違い: モデルは `AsyncData::KingmechModel`(`data/Enemy/ToiletMech/ToiletMech.mv1`)、`kModelScale` = 2.3(高さ約460)。剣(BossPirateWeapon)・部位破壊で落ちるパーツ・Z/X のデバッグ部位破壊は無し(`OnHeadBreak` / `OnPartBreak` はフラグだけ)。
- CSV:
  - `data/Enemy/BossEnemyKingmechAnim.csv`: 海賊ボスのアニメCSVと**同じ key** を全部持ち、中身を ToiletMech のアニメに割り当てた(Idle=Idle2、攻撃は Attack1〜5・Attack6_Shoot・Roar など)。
    ガード(Block系)・処刑される側(〜Victim)・回避のアニメは ToiletMech に無いので、Idle3・GetHit・KnockDeath・Walk で代用。`Roar` / `Shoot` / `Turn90L/R` / `Turn180` の key も足した(まだ使っていない)。
  - `data/Enemy/BossEnemyKingmechAttack.csv`: `BossEnemyPirateAttack.csv` のコピー(説明の文だけ変更)。**フレームの数字は海賊のアニメ用のまま**なので、判定の出るタイミング・止めるフレームは合っていない。ゲームで見て直す。
  - `EnemyStatus.csv` の `BossEnemyKingmech` 行: `BossEnemyPirate` 行のコピー。体・やられ判定の半径を 75→100、leftArmFrame=20(upperarm_l)、headFrame=123(head)。ガード率は0のまま(ガードのアニメが無い)。
- `EnemyType::BossEnemyKingmech` / 配置の type `BossEnemyKingmech`(StringToEnemyType・EnemyManager::CreateEnemy)。Unity の `EnemySpawnType` にも追加した。
- 配置: `Enemies003.csv` のフェーズ8(海賊と同じ 0,-1170,31500・180度・Idle・group Boss)。**R1(第二ラウンド)の行は消した**。
  遷移 `7 AllDead→8 / None`(R0 → Kingmech)、`8 AllDead→-1 / GameClear`。海賊(R0)は Kingmech の前の中ボス扱い。R1 のクラス・CSV はコードに残っている。
- F4: ボスのフェーズ中に押すと次のボスへ(R0 → R1 → Kingmech の順で、配置に無いものは飛ばす。今は R0 → Kingmech)。
- (追記)**海賊のボスのアニメ60個を Kingmech に足した**(Humanoid なので流せる)。mv1 のアニメは 40+60=100 個。フレームの番号は変わっていない。
  - `ToiletMechExport.Export` で `Assets/BossAnim.controller`(BossPirate.mv1 を作ったときのコントローラー)のクリップを足す。名前がかぶるもの・Humanoid でないものは飛ばす(今回は0個)。
  - FBX が約320MB になり、ビューアーの変換に数分かかる(待つ時間を長くすること)。
  - アニメCSV: **最初の版(メカのアニメ)のまま、ガード(Block系の5つ)だけ海賊のアニメ**(Block_Start / Loop / Hit / Hit_Break / End)。
    ほかの海賊のアニメは mv1 に入っているだけで、CSV からは使っていない(使うときは animName に海賊のアニメ名を書く)。攻撃CSVのフレームは海賊用のままなので、メカの攻撃アニメとは合っていない。
  - ガードのアニメができたので、ガードさせたいならステータスの guardRate を上げればよい(今は0)。
- MSBuild(Debug x64)でビルド・リンクは通った。アニメCSVのアニメ名が全部 mv1 にあることは確認。**ゲームでの確認はまだ**(向き・大きさ・当たり判定・攻撃のタイミング・首の向き)。

### 2026-10-28 F4 のボス探しを遷移表(フェーズ)で行う
- `DebugSkipToBossPrevPhase`: 敵の種類で探すのをやめ、遷移表の `BossAppear` の行をボスへ進む行とする(`FindEnemyPhase` は削除)。
  - 今のフェーズがその行の to 以上(ボス戦中)なら、今のフェーズから出る行の to(>0)へ飛ぶ(敵を消す、プレイヤーはそのまま)。それ以外は今まで通り from へ飛んでエリアの手前に移す。
  - ボスの種類を変えても F4 が効く。遷移表に BossAppear が無いステージ(PhaseStage004 など)では何もしない。フェーズ番号はボス戦以降が小さくならない前提。
- MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-28 敵の攻撃CSVで予備動作をなくせるようにした
- `preAnimName` が空欄なら、EnemyAttackPre の Enter で予備動作のアニメに切り替えない(前は GetAnimName の assert で止まった)。
- `preTime` / `preLookTime` は空欄なら0(前は数字が必須)。
- **予備動作なしの書き方**: preAnimName〜preLookTime を全部空欄。AttackPre を1フレームだけ通り、プレイヤーの方を向いてから攻撃する(攻撃権もそのまま)。
- CSV の2行目(説明)はまだ書き換えていない。MSBuild(Debug x64)でビルドは通った。ゲームでの確認はまだ。

### 2026-10-28 Kingmech のアニメCSVを作り直した
- `BossEnemyKingmechAnim.csv` = **海賊の `BossEnemyPirateAnim.csv` の60行そのまま**(key・アニメ・速度とも同じ) ＋ **メカのアニメ40行(key = mv1 のアニメ名)**。
  - コードが使う key(Idle / Walk / Hit / Block〜 / KnockDown〜 / 〜Victim など)は**海賊のアニメが流れる**(ユーザーが選んだ)。メカの動きにしたい key は animName をメカのアニメに書き換える。
  - メカの `Run` は海賊の key `Run` と重なるので key を `Run_Mech` にした。
  - アニメ名が全部 mv1 にあること、攻撃CSVの key が全部アニメCSVにあることは確認した。
- 攻撃CSV `BossEnemyKingmechAttack.csv` はユーザーがメカの key に書き換えた(下の表)。予備動作はダッシュアタック以外は空欄(予備動作なし)。
  **フレームの数字は海賊の攻撃のまま**なので、判定・止めるフレームはゲームで見て直す。

### Kingmech の攻撃(10/28 決定)
| index | 名前 | アニメ | 中身 |
|---|---|---|---|
| 0 | 横なぎ | Attack1 | |
| 1 | 範囲攻撃 | Attack2 | (決定のリストには無い。残すか決める) |
| 2 | 踏みつけ | Attack3 | |
| 3 | 地面パンチ | Attack4 | 前方へ一直線に地面からとげ・炎が順に噴き出す(Wave)。中距離用。nextAttack=4 のまま |
| 4 | ダッシュアタック | Attack5 | 突進。予備動作 AttackPre1(海賊のアニメ)。nextAttack=5 のまま |
| 5 | 火炎放射 | Attack6_Shoot | **特殊**。モデルを回転させて全体攻撃 |
| 6 | ミサイルの雨 | Roar | **特殊**。背中のパックからミサイルを撃ち、プレイヤーの周りに降らせる |
- nextAttack(3→4→5)は海賊のコンボのコピーのまま。連撃にしないなら -1 にする。
- **特殊攻撃の作り方(案、まだ作っていない)**: 攻撃CSVの最後に `special` 列(空欄=普通、`FlameSpin` / `MissileRain`)。
  `EnemyAttackPre.cpp` で `EnemyAttack` を直接作っている所を `owner->CreateAttackState()`(仮想関数、標準は EnemyAttack)にし、
  Kingmech で special を見て専用のステート(`EnemyAttack` を継承 → 攻撃の流れ・攻撃権・怯まない処理がそのまま使える)を返す。
  予備動作・攻撃力・吹き飛ばし・後隙・CT・attackRange は今の列を使い、専用の値(回る速さ・ミサイルの数など)は最初はステートの .cpp の定数。
- エフェクトの案(会話で出したもの): 地面パンチのとげ・炎の Wave、火炎放射、ミサイルと予告の円。Effekseer MCP で作る。
- (10/28) Kingmech は基本追いかけない・攻撃のサイクルが速い・技の選び方も変える予定。
  `BossEnemyKingmech.h/.cpp` に Next〜 の override を全部足した(NextAfterIdle / NextOnArrived / NextAfterCaution / NextAfterAttack / NextAfterParry / NextOnPlayerFound / NextAfterRecover。NextAfterAttackEnd は前から)。
  **中身はまだ EnemyBase を呼ぶだけ**(動きは変わらない)。ビルドは通った。
  - 気をつけること: 攻撃権の全体の間隔 30F(`AttackTokenManager` の kGlobalAttackInterval)・Idle の長さ(ステータスの idleTime)・CT もサイクルの速さに効く。
    EnemyIdle はプレイヤーの方を向かない(ToPlayerLook がコメントアウト)。Update の「被弾中に CT が明けたら攻撃」は Next〜 を通らず EnemyAttackPre を直接作っている。

### 2026-10-29 Kingmech のガトリング(部位破壊で落とす)のモデル
- 方針(会話で決めた): ガードブレイクで**右腕のガトリングを切り落とし、回して投げ返して爆発**(相手の弾薬で爆発する、という納得感)。腕まるごとはやめた。
- **モデル**: `data/Enemy/ToiletMech/KingmechGatling.mv1`(骨・アニメ無し、テクスチャは同じフォルダの `Texture/` の体のもの)。まだコードからは使っていない。
  - 元は ToiletMech.mv1 のフレーム `polySurface789`(メッシュ1個、ガトリングだけ。腕は別のメッシュ)。骨は `lowerarm_r` にほぼ100%。肩へのケーブルは外した。
  - 頂点は **lowerarm_r の空間(回転だけ)**、**原点はガトリングの真ん中**(回すときの中心)。大きさ 約114×35×24(倍率1のとき)。
- **ゲームでの使い方**:
  - ボスのガトリングを消す: `MV1SetMeshVisible(ボス, MV1GetFrameMesh(ボス, MV1SearchFrame(ボス, "polySurface789"), 0), FALSE)`(前腕は残る)。
  - 切った瞬間の置き場所: `MV1SetMatrix(ガトリング, MMult(MGetTranslate(VGet(-52.542f, -5.352f, 3.009f)), MV1GetFrameLocalWorldMatrix(ボス, lowerarm_r)))`。
    ボスの kModelScale(2.3)もこの行列に入っているので、そのあとも同じ倍率を掛けて描く。Idle2・Attack1・Attack6_Shoot でボスのガトリングにぴったり重なることを画像で確認した。
- 作り方: `tools/KingmechGatling/`(build.bat で `build.bat extract` など)。`extract.exe ToiletMech.mv1 出力フォルダ` → KingmechGatling.mv1(.x は消した)。
  オフセットは extract_log.txt の center。`verify.exe ボス.mv1 ガトリング.mv1 出力フォルダ` で重ねた画像(切り出した方を赤く)。probe / weights / shot は調べる用。

### 2026-10-29 ガトリングを切り落として投げ返す(BossPartThrow)
- 流れ(**切り上げが出てからは全部自動**): ガードブレイク → チャンス中に Y → 打ち上げ(GuardBreakAttack0) → 切り上げ(GuardBreakAttack1)が Kingmech に当たる
  → ガトリングが切れて跳ね上がり、プレイヤーの左手へ寄って回る → 切り上げのアニメが終わったら `PlayerStateBossPartThrow`(空中で止まって空中弱5のモーション)
  → 弱5の攻撃判定の開始フレーム(ComboChain の attackColStart の1つ目)で投げる → ボスの spine_03 へ飛んで爆発 → プレイヤーはモーションが終わったら Fall。
- `Character/Enemy/Kingmech/Part/KingmechGatling.h/.cpp`: ボスが Init で作って持つ(shared_ptr)。ガンナーの銃と同じく持ち主のボスを weak_ptr で持ち、ボスのモデル・プレイヤーはボスから取る。爆発したらボスの `OnGatlingExploded()` を直接呼ぶ(friend)。Phase = Attached / Cut / Hold / Thrown / Gone。当たり判定なし(爆発はボスとの距離)。
  - Cut でボスのガトリングのメッシュを消し、同じ位置・向き・大きさから始める。定数(跳ね上がり・手へ寄せる長さ・回る軸と速さ・投げる速さ・爆発の距離)は .cpp の先頭。
  - 持たせる手は `hand_l`(右手は刀)。回す軸 `kSpinAxis` はモデルの空間の Y(見て直す)。
  - 爆発のエフェクトは**仮でスキル3の着地(GhostSkill3ImpactEffect)**。専用の爆発を Effekseer で作る。
- `Character/Enemy/Kingmech/State/BossPartThrow.h/.cpp`(敵): 膝をついて待つ(KnockStart → KnockLoop)。爆発したら KnockEnd で立ち上がり NextAfterRecover。
  600F 待っても投げられなければボスが自分で投げさせる。この間の被弾はダメージだけ(HPは1残す)。
- `BossEnemyKingmech::OnDamage`: `data.comboIndex == GuardBreakAttack1` かつガトリングが付いていたら `CutGatling`(ダメージ・部位破壊フラグ・BossPartThrow・`player->RequestBossPartThrow`)。
  爆発で `kGatlingExplodeDamage`(300)。HP が0なら EnemyDie。
- `EnemyBase::ThrowPart()`(仮想、標準は何もしない)。プレイヤーは `m_bossPartThrowTarget` を見て、切り上げの入力・空中コンボへの派生をせずに投げに行く。
  `PlayerStateBossPartThrow` は無敵。途中で抜けても Exit で投げる。
- `AsyncData::KingmechGatlingModel` を追加。MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**
  (切れる位置・手に寄る速さ・回り方・投げのタイミング・爆発の位置。ボスのガード率が0なのでブレイクさせるにはステータスの guardRate を上げる)。
- まだ: ガトリングを失った後に銃の技(Attack6_Shoot など)を出さないようにする、専用カメラ。

### 2026-10-29 列車ステージの明るさに差をつけた(コントラストが薄い・地味と言われた対策)
- 戦う場所を明るく、トンネルと遠くを暗くした。値は `Stage.cpp` 先頭(`kTunnelColorScale` 0.45 / `kTrainColorScale` 1.15 / フォグ 2000〜5000、色 8,8,10)。
  - `Stage::GameInit`(トンネルがあるとき): トンネルの環境光・拡散を×0.45、列車の環境光だけ×1.15(拡散は血殺のフォグが MV1SetDifColorScale で使うので触らない)。
  - `Stage::Draw`: 列車とトンネルを描く間だけ暗いフォグ。血殺のフォグがかかっているとき(`GetFogEnable()`)はそちらを優先。キャラ・エフェクトにはかけない。
  - mv1 は変えていないので、Unity から書き出し直しても効く。マテリアル名が M0〜 で床だけを選べないので、列車は全体を明るくしている。
- 見た目の確認は preview.exe の改造版(scratchpad)で、ゲームと同じライトで比べた。MSBuild(Debug x64)でビルドは通った。**ゲームでの確認はまだ**。

### 2026-10-29 ミサイルの雨のエフェクト(Effekseer MCPで制作)
- 場所: `data/Effect/Kingmech/`(`Texture/`・`Model/`)。血しぶきと同じ **1/20 スケールで作成、ゲームでは20倍で再生**。設計は `../進捗/note_ミサイルの雨の設計.md` の段階に合わせて3つ。まだコードからは使っていない(System の AsyncData にも未登録)。
  - `MissileWarning.efk`(Warning 45F): 着弾位置の地面に赤い予告の円。外周の輪＋目盛り、外側で回る破線、中の塗り(だんだん濃く)、**中心から広がる橙の円が45Fで外周に届く**(=着弾までの残り時間)、30F〜で3回点滅。20倍で半径約190。
  - `MissileFall.efk`(Falling 15F): 高さ60(=1200)から速度4/Fで真下に落ち、**15Fでちょうど y=0**。自作のミサイルのモデル(先端が赤・黄の帯・灰色の胴・尾翼4枚)＋噴射の光と炎＋光の尾＋煙の柱(着弾後30Fほど残って消える)。再生位置は着弾点(地面)。
  - `MissileExplode.efk`(Explode): 閃光(5F) → 火球(〜28F) → 黒っぽい重い煙が上がって約1秒で消える。地面に衝撃波の輪・橙の光・外へ広がる砂ぼこり・火の粉、焦げ跡(150Fで消える)。20倍で火球の半径約180。
  - 爆発の半径(kExplodeRadius)を変えたら、3つとも同じ倍率で拡大すれば予告の円と合う(20倍で半径約200 → 例: 半径150なら15倍)。
- 素材: テクスチャ `WarnRing / WarnFill / WarnDash / ShockRing / Scorch / SparkStreak` とモデル `Missile.efkmodel` は `tools/KingmechMissileFx/`(gen_fx_textures.ps1 / gen_missile_model.ps1、PowerShell。**BOM付きUTF-8で保存すること**)で作った。
  `MissileSmoke / MissileFire / MissileGlow` は Effekseer 付属サンプル(01_Pierre01 の Smoke / Fire / Particle_Soft)のコピー(煙と火は2×2のシート)。
- 編集用の `.efkefc` も同じフォルダにある。**ゲームでの確認はまだ**(大きさ・暗さ・煙が多すぎて避けにくくないか・ミサイルのモデルがDxLibで出るか)。
- 組み込み(10/29、ビルドは通った・ゲームでは未確認):
  - AsyncData に `KingmechMissileWarningEffect / FallEffect / ExplodeEffect` を追加して System で読み込み。
  - `Kingmech/Part/KingmechMissile`(新規): Warning 45F → Falling 15F → Explode 4F → Finished。段階が変わるときにエフェクトを20倍で再生、着弾で予告の円を止める。タイマーはタイムスケールで進める。**攻撃判定はまだ無い**(FixedAttackCol が未作成)。
  - ボスが `m_missiles` を持ち `FireMissile(landPos)` で追加。Update で回して終わったものを消す。ボスが消えたら残りも消す。
  - **デバッグ: M キーでプレイヤーの足元に1発**(F は敵を出すのに使用中)。
  - 次: ゲームで見る → FixedAttackCol で判定 → 攻撃ステート(KingmechMissileRain)で複数発。

---

## 10/27 やること: 背中の腕の切り離し Step2〜
**手順とコードは全部 `../進捗/背中の腕の切り離し_手順.md` に書いてある。** ここは順番と進み具合だけ。

今の状態(10/27): 1-1〜1-5 は済み(1-1・1-2 はユーザー、1-3〜1-5 は Claude。ビルドは通った)。次は 1-6(ゲームで確認)。

- [ ] **Step2 腕を切り離して上げ、戻す(見た目だけ)**
  - [x] 1-1 `BossArm.h` の private に関数の宣言3つ(`CalcDetachedSpineMatrix` / `CalcDrawSpineMatrix` / `LerpMatrix`)
  - [x] 1-2 `BossArm.cpp`: include、Update の位置、Draw の背骨、ReturnToIdle、追加の関数5つ(`Detach` / `RequestReturn` / 上の3つ)
  - [x] 1-3 `BossArmDetached`(新規、`BossPirate/State/`): 回りながら300上がって浮く
  - [x] 1-4 `BossArmReturn`(新規、`BossPirate/State/`): 30Fで背中へ戻って Idle
    - `std::min` は windows.h の min マクロとぶつかるので `(std::min)(...)` と書く(Animation.cpp と同じ)
    - ImGui の Boss Arm Editor の Idle ボタンを `ReturnToIdle()` にした(切り離し中に押しても背中に戻る)
  - [x] 1-5 `BossGuardBreak.cpp` の TODO のコメントを外す
  - [ ] (未解決)ガードブレイクのよろめき中に遷移する件。原因2つ(ガードブレイクアタックの例外・型の判定が BossGuardBreak)。手順書の「4.」(Enter で `Detach`、Exit で `RequestReturn`)
  - [ ] 1-6 ビルド → ゲームで確認(F4 を2回で R1、弱攻撃でガード → 強攻撃でブレイク。Guard ボタンでは腕は動かない)
- [ ] **Step3 浮いている腕に当たり判定**(Step2 が動いてから)
  - 先に決める: ブレイク後の Y の打ち上げはボスと腕のどちらを狙うか / 腕のダメージはボスの HP か別の HP か / 空中コンボ中に戻せと言われたら待つか
  - 調べた結果と案は手順書の「2.」(10/27)。要点: やられ判定(HitCol)だけ作る / 位置は腕の真ん中 / OnDamage は腕用 /
    `m_isFloor` の初期値が true なので切り離し中は `SetIsFloor(false)` / 空中で入力なしだと地上のボスを狙い続けるのを直す / 戻ったら内部ターゲットから外す
- [ ] そのあと: BossBase(R0 / R1 の共通部分をまとめる) → 攻撃2種の追加

---

## TODO / 次にやること
- [ ] ImGui のウィンドウをゲームの外に出してみる(描画が崩れないか・ゲームが止まらないか・キー入力)。Enemy Debug ウィンドウで攻撃を出して確認
- [ ] ボスの連続攻撃: コードはできた。BossEnemyPirateAttack.csv に1撃目・2撃目の行を書いてゲームで確認(向き・間の長さ・連撃中の被弾で止まってよいか)
- [ ] 追加したボスのアニメ(回避・コンボ)を攻撃CSV・ステートで使う
- [ ] ボスの剣をゲームで確認(握り・向き)。腰の剣(Pirate_Sword_In_Belt)を消すか決める
- [ ] ボスのラウンド切り替えの演出(7→8 のイベント。R0 の死体を消す・変身エフェクト・カメラ)
- [ ] 一時ライト(TempTopLight)を本番のライト調整ができたら消す
- [ ] BossArm: ゲームで付け根のずらしを ImGui で決めて定数に書き写す(今は0＝ボスの腕に重なる)。Anim の Play で腕だけ違うアニメが流れるか確認
- [ ] BossArm Step2: 当たり判定・ステータス行・腕の長さで手の位置を制限、敵のステート(Hit/HitAir/AirStay/AirFall)を腕で通す。本物の腕モデル(骨・アニメ入り)
- [ ] BattleSmoothCamera の戻る条件をゲームで確認(30F / 800 / 100 の調整)
- [ ] 落下禁止をゲームで確認(縁で止まる・辺に沿って滑る・角で止まる・攻撃・回避の後に歩けば落ちる)
- [ ] カメラとステージの当たり判定(案A)をゲームで確認。めり込みが気になれば案B(MV1CollCheck_Sphere の二分探索)へ
- [x] FOVパンチ(ゲームで強さ・長さを確認。場面ごとの強さ・向きは note の8章の表)
- [x] 視野角オフセット(戻すまでずっと)を CameraManager に入れた。今はボス戦で+20°だけ。ゲームで広がり方・20°が広すぎないか確認。ダッシュに使うなら FastRun の Enter/Exit で対にする
- [ ] ブレイクの打ち上げの続き: 敵を打ち上げる処理とCSV(ユーザー)・専用カメラ・演出。ゲームで一連の流れを確認
- [ ] **フェーズ進行の外部化**(遷移表CSV＋グループ指定＋EnterArea)。設計と手順は `../進捗/フェーズ進行_設計メモ.md`
  - [x] Step1 PhaseManagerへ移すだけ(ゲームでの確認はまだ) → [x] Step2 遷移CSV(AllDead) → [x] Step3 デバッグ表示(どちらもゲームでの確認はまだ)
  - [x] Step4 グループ指定/Timer/KillCount(コードは入った。遷移CSVではまだ使っておらず、ゲームでの確認もまだ)
  - [ ] Step5 EnterArea(途中)
    - [x] エリアCSV(`data/Stage/PhaseAreas/Stage003.csv`) → [x] `DataManager::LoadPhaseAreas`
    - [x] SceneMain で読んで `PhaseManager::Init` に渡す → [x] 遷移表のエリア名から `areaIndex` を引く(無ければ assert)
    - [x] `IsInArea` と `IsConditionFulfilled` の EnterArea → [x] 箱のデバッグ描画 → [x] エリアエディタ(ImGui)
    - [ ] ゲームで箱の位置を確認・エディタで直して保存 → [ ] 遷移CSVに EnterArea の行を書いて確認
  - [ ] Step6 イベント・ボス(仮)
- [ ] フォルダ整理後にゲームを起動して確認(特にシェーダー読み込み = 血殺中に敵が黒くなるか)
- [ ] 使われていない定数の掃除(EnemyAttackPre の kAttackPreBlendRate / kAttackPreTrackEndRate、EnemyParry の kAnimEndFrame / kAnimMoveEndFrame)
- [ ] Unityの配置CSVに、1体ごとの上書き用の列(HP・視界など、空欄なら標準値)を必要になったら足す
- [ ] 敵の攻撃データを各Stateで使う(AttackPre → Attack → AttackEnd → 攻撃距離 → 連撃の順)
- [ ] EnemyParry の残り(弾く条件・アニメ・弾き中の被弾の扱い)を決めて実装する
- [ ] PlayerAttackParried の残り(アニメ・必殺技/スキル中・空中で弾かれたとき)
- [ ] ゲームで後隙の長さ(40F)と、最後の姿勢で止まる見た目を確認。止まって見えるなら専用のアニメ(構え直し)を検討
- [ ] ゲームで確認: 敵が倒れるか(地上/叩きつけ/空中)、攻撃が背中側に当たらないか、アニメ速度(二重更新を直した影響)
- [ ] ゲームで攻撃権を確認(敵3〜4体で同時に予備動作に入るのが2体までか、30Fずれるか、中断・死亡で返るか)。デバッグ表示(HasToken / GetUsedCost)を足すと見やすい
- [ ] 攻撃権の待機ステート(EnemyAttackWait)を検討(攻撃距離より外側で周回し、空いたらすぐ攻撃)
- [ ] ゲームで予備動作の長さ・向き固定のタイミング(kAttackPreTrackEndRate)を確認。AttackPre2 も使うか検討
- [ ] 実際のUIクラス(HPバーなど)を作って `InGameInit()` に登録する
- [ ] ゲームを起動して血殺中に敵(本体・取れた腕/頭)が黒くなるか確認
- [ ] ゲームを起動して見た目を確認(フォグの距離・ステージの暗さ、血の大きさ(20倍)、向き、白い血、スロー時の速さ)
- [ ] 多段ヒットで血が出すぎないか確認。多すぎるなら一定フレーム出さない制限を入れる
- [ ] 血殺演出の残り(2D極太斬撃、3Dの白い斬撃、「血殺」の文字、端を黒くするビネット、振動)
  - 描画順の案: 背景(赤) → ステージ(赤フォグ) → キャラ → 2D斬撃 → 3D白斬撃 → 血しぶき → ビネット → 文字
- [ ] フォグで色が濁るようなら、ステージ用のグラデーションマップシェーダーを検討
- [ ] ルートの CLAUDE.md の記載が `2026_summer/` のままなので、`2026_winter/` に直す
- [ ] **列車ステージ(Ch0 0-1)をゲームで確認**(ステージセレクトの3番目「列車ステージ」。StageType::Train)
  - 見た目 `data/Stage/TrainStage/TrainStage.mv1`、当たり判定は箱とスロープだけの `TrainStageCol.mv1`(StageInfo::colModel)。トンネル `data/Stage/TrainTunnel/TrainTunnel.mv1`(13200でループ)を `Stage::DrawTunnel` でカメラの前後6000だけ描き、+Z へ流す(速さ kTunnelScrollSpeed=20/F、y=-150)
  - 編成(前から): 機関車 / 1両目 平台車(オレンジのコンテナ) / 2両目 平台車(さびたコンテナ) / 3両目 通路の車両(中を通る) / 上りスロープ車 / 4両目 通路の車両(扉を閉めて屋根を歩く、屋根y=602、赤いさびの波板) / 下りスロープ車 / 5両目 通路の車両(中を通る) / 6両目 平台車(最後尾)。全長約235m。デモステージと同じくプレイヤーは+Zへ進む(機関車がZ=-7900、最後尾がZ=23400。組み立て後に180°回している)。幅は平台車・通路の車両とも約16m
  - 通路の車両は宇宙船の通路(spaceship_corridor_blend)を横3倍・高さ1.9倍にしたもの(天井・入口は約6m)。トンネルは拡大率11で横だけさらに2倍(幅約44m、天井はデッキから約14.7m、1区間33m。build_all.sh と build_train_stage.py の TUNNEL_SCALE・TUNNEL_X_SCALE をそろえる)。ノードの材質は `prepare_corridor.py` で画像に焼いて `corridor_car.blend` に保存。窓はガラスを外し、外側の壁(縦の部分)と屋根に赤いさびの波板(さびたコンテナのテクスチャから切り出し)を貼ったので、窓からは赤い壁が見える。SF木箱は置いていない。床の切れ目(各車両の前後の端・平台車の左右の縁・スロープの上り口と下り口・屋根の前後の端)には黄黒のテープ(hazard.png、見た目だけ)
  - 座標: デッキ上面 y=0、原点=スタート(1両目の前寄り)。前からの距離u(m) → Z=(u-1.5)*100、X は組み立て時の符号の逆
  - 色: ステージとトンネルは `convert.exe flat` でキャラ・デモステージと同じ「拡散=色×0.1・環境=色×0.9・反射0.1」にしてある(ライトの向きで明るさが変わらず、敵とのなじみがデモステージと同じ)。色は build_train_stage.py の TINT_*、トンネルは build_all.sh の TUNNEL_DIF
  - 敵配置 `Enemies004.csv`・遷移 `PhaseStage004.csv`(A:1両目 → B:2・3両目 → C:4両目の屋根と5両目 → D:6両目 → GameClear)は仮。エリートは今 Swordman
  - 列車の左右の縁(x=±800)と前後の端に、高さ1200の見えない壁を立てて外へ出られないようにしている。連結部のすき間に落ちると線路の高さ(y=-170)の見えない床に立つ
  - 作り直し: `tools/TrainStage/build_all.sh 素材フォルダ [--corridor]`(Git Bash)。中身は prepare_corridor.py → build_train_stage.py → build_tunnel.py → `tools/FbxToMv1/export_x.py`(.x) → `convert.exe`(mv1)。ゲームのDxLibはFBXを読めないので .x 経由。`preview.exe` でゲームと同じライトの見た目を PNG に、`probe.exe` で当たり判定の高さ・法線を調べられる
  - **Unityで調整する(今後はこちらが本番)**: Unityプロジェクト `Desktop/programm/ninja_motion` の `Assets/Editor/TrainStageTool.cs`。
    `Tools > Train Stage > 1. Import From Game Data (Setup Scene)` で `data/Stage/TrainStage/TrainStage_Unity.fbx`・tex・トンネルを取り込んで `Assets/Scenes/TrainStageScene.unity` を作る(作り直すと調整は消える)。
    `TrainStage`(見た目、車両ごと・コンテナは1個ずつ)と `TrainStage_Collision`(当たり判定、箱1個ずつ・半透明の緑)を動かし・拡大・複製・Cube追加して調整し、
    `Tools > Train Stage > 2. Export To Game (mv1)` で .x → `convert.exe flat` → ゲームの TrainStage.mv1 / TrainStageCol.mv1 を上書き。マテリアルの色・テクスチャ(Assets/TrainStage/Materials)の変更も入る。
    `TrainTunnel_Reference` は目安(EditorOnly、書き出さない)。Unityの座標×100がそのままゲームの座標(+Zへ進む、原点の床はy=0のまま)。
    **build_all.sh を実行すると Unity での調整が上書きされる**。Unity経由の mv1 は Blender 経由と当たり判定・見た目が一致することを確認済み
    手順書は `tools/TrainStage/Unityでの手直し.md`。使った素材は `export_props.py` で Unity の `Assets/TrainStage/Props` にFBX化してあり、`Tools > Train Stage > 3. Setup Props` で `Props/Prefabs` にプレハブ(12個)。
    `Add Collision Box For Selection` で選んだ物の外形の当たり判定を作れる。2. Export は敵配置(EnemySpawns)も Enemies004.csv に書き出す(1. Import で読み込む)
  - Unity の敵配置ツールを直した: `EnemySpawnInitialState.Guard` → `Watch`(ゲームの名前。保存値は同じなのでデモの敵もそのまま)、`EnemySpawnType` に `BossEnemyPirateR1` / `BossEnemyPirateR0` を追加、
    `Tools > Enemy Spawn > Import CSV...`(ゲームのCSVをシーンに読み込む)を追加。デモステージの Enemies003.csv も Unity から書き出し直せるようになった
