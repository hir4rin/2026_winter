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

---

## TODO / 次にやること
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
