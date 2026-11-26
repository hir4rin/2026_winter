# 強攻撃の溜め中(Y 長押し)のエフェクトの案

案の中身(動くモックアップ)は `../../tools/ChargeHoldMock/charge_hold_mock.html`。そこから A・C・D・E・F・H・J・K・L を Effekseer で作った(11/26)。**まだどれにするか決めていない**。
A は今の溜め(`../ChargeAttack/A_Charge.efkefc` → `data/Effect/ChargeAttack/Charge.efk`)のまま。

## ゲームでの確かめ方
- **F11 で案を切り替える**(A → C → D → E → F → H → J → K → L → A)。切り替えると左上に名前が2秒出る。次に Y を長押ししたときからその案が出る。
- 書き出し: `data/Effect/ChargeHold/〇_Hold.efk`(テクスチャは `Texture/`、モデルは `Model/`)。
- コード: `AsyncData::ChargeHoldCEffect`〜`ChargeHoldLEffect`(並びは `System.cpp` の `kChargeHoldPaths`)、`System::CycleChargeHoldType / GetChargeHoldEffectKey / GetChargeHoldName`、`SceneMain` の F11、`PlayerStateCharge::Enter` で再生。
- F の案だけ、溜め完了(15F)で `PlayerStateCharge` が画面を揺らす(`kChargeDoneShake〜`)。
- 決まったら: 選ばなかった案の AsyncData・パス・F11 の処理・揺れの分岐を消し、選んだものを `Charge.efk` に置き換える。

## 作り
- 1/20 で作り、ゲームでは20倍。+Z が前。座標の決め方は `../ChargeAttack/README.md` と同じ(刀の根元 (1.54,3.72,2.29)・先 (-0.97,2.18,0.20))。
- 作業ファイル: `〇_Hold.efkefc`(どれも `Debug` ノードに確認用のプレイヤーの形 `DebugPose` が入っている。**書き出す前に消す**)。`_Clean.efkefc` は確認用の形だけの土台。
- **プレイヤーの溜めの構えのモデル**: `Model/CH_PoseLoop_b.efkmodel`(刀ごと、`Power_Attack_Loop` の0フレーム)と、法線の向きに 0.05 ふくらませた `CH_PoseShell_a.efkmodel`。
  作るツールは `../../tools/PlayerAnim/posemodel.cpp`(`build_posemodel.bat`)。`posemodel.exe 出力 Player03.mv1 刀.mv1 アニメ名 コマ 0.05 [ふくらませる量]`。頭の位置などは `posemodel_log.txt`。
  - **DxLib と Effekseer は奥行き(Z)が逆**なので、ツールで Z を反転している。
  - 頂点 17万・11MB と重い。J・K・L はこれを数回描く。
- **ふちの光**(J・K・L): ふくらませたモデルの裏面だけを加算で描く(カリング `Back`。Z を反転したので Effekseer の表裏の呼び方が逆になっている。`Front` にすると体全体を覆った)。
  エディタでは確認用の形が深さを書かないので体全体が光って見える。ゲームではプレイヤーが先に描かれるので輪郭だけになる。
  ポーズは止まっているので、構えはじめ(ChargeStart)と、ループの体の上下でずれてはみ出す。
- 素材は `gen_chargehold_tex.ps1`(BOM 付き。白い画像で色はノードで付ける): `CH_Streak`(風の筋)/ `CH_WindArc`(床の風の弧)/ `CH_ShockRing`(輪)/ `CH_Lightning`(稲妻 2×4)/ `CH_Rocks`(岩 2×2)/ `CH_Flame`(炎の舌)/ `CH_Wisp`(気の筋 4×1)/ `CH_Crack`(ひび)。`CA_Glow / CA_Smoke / CA_Star` は溜め攻撃と同じもの。
- 床に置くもの(輪・弧・ひび・床の光)は `DepthOffset` 40(エディタでは見えなくなる)。

| 案 | ノード | 中身 |
|---|---|---|
| C 風を呑む | InBurst / InLoop / DustIn / BurstRing / BurstDust / Swirl1〜3 | 筋が体へ吸い込まれる(方向ビルボード)、砂ぼこりが寄る、15Fで床の輪と土煙、以後は床の弧が回る |
| D 紅の闘気(v2) | 溜め中: GatherIn(Ember) / FlameBuild。溜まった後: FlareBurst / FlareGlow / DoneRing / TipStar / FlameRed / FlameDark / FlameCore / AuraGlow / BladeGlow | **溜め中と溜まった後を分けた**(下) |
| E 赤い雷 | BoltBuild / BoltBuild2 / BoltLoop / BurstPivot / BladeGlow / Flash / GroundGlow | 刀身に稲妻(間隔が詰まる)、15Fで胸から9本弾ける＋閃光、以後は刃が赤く光り時々稲妻 |
| F 重圧・地割れ | CrackGrow / CrackStay / CrackRed / RockPivot(Tremble・Float) / DustRing | ひびが広がる、小石が震え15Fで浮く、ひびの奥が赤い、土煙の輪。揺れはゲーム側 |
| H 鬼の眼光 | EyeGrow / EyeStay / EyeTrail / EyeStar / BladeGlint | 目(1.05,4.95,3.2)が光り後ろへ尾、15Fで横長の光＋刃に白い光。**後ろからのカメラでは目は頭に隠れる**(尾だけ見える)。画面を暗くする所は作っていない |
| J 残像が重なる | Ghost1〜4 / Jitter / ShellFlash / ShellStay | 左右から赤い半透明の残像が集まる、15Fで輪郭が光る、時々ずれた残像 |
| K 青→赤 | PreGlow / S1(15F〜60F) / S2(60F〜) | 動画(NG4 絶技集の冒頭)の型。気の筋が弾ける(Track)・立ちのぼる筋・稲妻・床の渦・輪郭の光。S2 は赤で多く、画面いっぱいのフレア |
| L 白銀→紅 | K と同じ | K の1段目を白銀にしたもの |

- **D(v2、11/26)**: ユーザーが D 寄りで、「溜め中と溜まりきったときをわかりやすく分けて」。
  - 溜め中(0〜15F): 赤い筋の火の粉が体へ吸い込まれる(GatherIn、C の筋と同じ作り・36本で12Fまで)、足元だけに低く暗い赤の小さな炎(FlameBuild、40個で15Fまで)。
  - 溜まった瞬間(15F): 燃え上がる炎(FlareBurst)・大きな赤い光(FlareGlow)・床の赤い輪(DoneRing)・切先の十字の光(TipStar)。
  - 溜まった後: 全身に明るく高い赤い炎(FlameRed、前より多く)＋黒い炎(FlameDark)＋中に橙の芯(FlameCore)、体の赤い光(AuraGlow)、刃が赤く光り続ける(BladeGlow)。**炎が出るのは15Fからだけ**。
  - 前の版は `old/D_Hold_v1.efkefc`・`.efk`。ゲームで撮った画像は `game_D_v2.png`。
- **A の方向で詰めた(A2〜A5、11/26)**: ユーザーが「A の方向で、チャージとチャージ完了をわかりやすく」。今の A は溜め中も完了後も黒い瘴気が同じで、完了の合図が細い赤い光だけだった。
  - 共通(`_Acommon.efkefc`、元は `_Base.efkefc` = A のコピー): 完了(15F)で 切先の大きな十字の光(TipStar を19F→15F・大きさ32)、刀全体の赤い光(BladeGlow を太く)、刀の中ほどの赤い光(RedFlash)、刀から黒い瘴気が弾ける(BurstSmoke)。完了後だけ 赤黒の瘴気(Miasma)・火の粉(BladeEmber を15Fから)・体の赤いもや(AuraRise)。
  | 案 | 溜め中 |
  |---|---|
  | A2 吸う→噴く | 黒い瘴気と赤い筋が刀へ吸い込まれる(InMiasma の Puff / Streak、42個で12Fまで)。完了後の瘴気は強く噴き上がる |
  | A3 刃が黒く染まる | 鍔から切先へ1Fに1つずつ黒い光が並び(BladeStain)、その先頭に赤い点(StainHead)。煙は少し。**後ろからのカメラだと刀が体の前で見えにくい** |
  | A4 煙の色が変わる | 灰色の煙だけ(BladeSmoke を灰色で15Fまで) → 完了で赤黒の瘴気に |
  | A5 A2＋A3 | 吸い込み＋刃が染まる |
  - 比べた画像は `game_A_variants.png`(一時コードで撮った)。F11 の順は A, A2, A3, A4, A5, C, …。
- **D の別パターン(D2〜D5、11/26)**: D v2 が「結構微妙」だったので、溜め中と溜まった後の分け方を変えて4つ足した。どれも D v2 の作業ファイルから作った(`D2〜D5_Hold.efkefc`)。F11 で D の次に D2 → D3 → D4 → D5 の順。比べた画像は `game_D_variants.png`。
  | 案 | 溜め中 | 溜まった後 | 分かれ目 |
  |---|---|---|---|
  | D | 赤い火の粉が吸い込まれる＋足元が暗く燃える | 全身の明るい炎＋橙の芯＋刃の赤 | 吸い込む → 吐き出す |
  | D2 | 黒い炎(Blend)と黒い煙が全身から立つ | 黒い炎が主、中に細い赤い炎(赤黒) | 色 |
  | D3 | 炎が 足元(0F〜)→腰(5F〜)→胸(10F〜)と背を伸ばす(FlameBuild1〜3) | 1.4倍の大きな炎＋噴き上がる火の粉(SparkUp) | 大きさ |
  | D4 | 刀だけが燃える(刀の線から炎・刃がだんだん赤く) | 刀から炎が飛び散り(BladeBurst)、刀が大きく燃え続ける(BladeFire)。体の炎は少なめで17Fから | 燃える場所 |
  | D5 | 赤い輪が3回、体へ縮む(PulseIn)＋体の光が脈打つ(BodyPulse) | 炎は少なめ、40Fから30Fおきに輪と光が脈打つ(BeatRing / BeatGlow) | リズム |
  - 溜まった瞬間(15F)の燃え上がり・床の輪・切先の光は全部共通(D4 だけ体の燃え上がりが17F)。
- **A2 を3つに分けて、ゲームのフレームで出し入れする形にした(11/26)**。ミサイルの予告円(`KingmechMissile`)と同じ考え方。完了のタイミングがコードの `kChargeThreshold`(今は40F)と必ずそろう。
  | ファイル | ノード | 出し方 |
  |---|---|---|
  | `A2_Charging` | InMiasma(生成数を無限にした) | Enter で再生。完了で `StopRoot`(出ている煙は自然に消える) |
  | `A2_Done` | TipStar / RedFlash / BurstSmoke | 完了の瞬間に1回 |
  | `A2_Charged` | BladeGlow(寿命 36000)/ Miasma / BladeEmber / AuraRise | 完了から離すまで |
  - どれも出始め(GenerationTimeOffset)は0。書き出しは `data/Effect/ChargeHold/A2_Charging/Done/Charged.efk`。
  - コード: `AsyncData::ChargeChargingEffect / ChargeDoneEffect / ChargeChargedEffect`、`PlayerStateCharge` の `Phase`・`ChangePhase`・`PlayEffect`。**溜め中はもう F11 の案を使わない**(F11 の処理は残っているが効かない)。F の案の揺れは外した。
  - ゲームで撮った画像は `game_A2_phase.png`(12F・30F 吸い込み、42F 完了、60F〜 溜まった後、125F 離した)。
- K・L の段は親(None)の `S1` / `S2` の出る時間で分けている(S1 の寿命 45 で 60F に消える)。2段目の 60F は見た目だけ(ゲームの溜めは1段)。
- ゲームで撮った画像: `game_sheet_0.png`(A・C・D・E・F)/ `game_sheet_1.png`(H・J・K・L、カリングを直す前)/ `game_JKL_culling.png`(直した後)。
- **O4「黒から赤へ(二層)」にした(11/26)**。案はオーラのモック `../../tools/ChargeHoldMock/charge_aura_mock.html` の O4。A2 の3分割と同じ出し方で、ゲームのパスを `O4_Charging/Done/Charged.efk` に差し替えた(A2 のファイルは残してある)。
  | ファイル | ノード | 中身 |
  |---|---|---|
  | `O4_Charging` | BodySmoke(0F〜) / BodySmoke2(16F〜) / BodySmoke3(28F〜) / LegSmokeF・B | 黒い煙だけ。段ごとに足されて濃く・大きくなる(赤は無し) |
  | `O4_Done` | RedFlash / FlameBurst / ShellFlash / TipStar / BurstSmoke | 胸の大きな赤い光＋体の輪郭が赤く光る(CH_PoseShell_a)＋赤い炎が一気に出る |
  | `O4_Charged` | BodySmoke / LegSmokeF・B / BodyFlame / ShellRed / BladeGlow / BladeEmber / AuraRise | 外が黒い煙、内が赤い炎(CH_Flame)の二層 |
  - 体のまわりに出すのは**球(Sphere)で、角度 RotationX/Y を ±180 にする**(0 のままだと一方向にしか出ず、細い柱になった)。中心は PVA の Location(x 0.5・y 0.6〜4.8・z 1.0)。モデルの面から出す(GenerationLocation=Model)と、髪の面が多いので上半身に偏った。
  - 構えのモデルの座標: エディタの横から(angleY -90)の画面で、左が +Z(MarkFront が z=8)。前の足 z≈3.4、後ろの足 z≈-1.9、頭 y≈5.2。`posemodel_log.txt` の値とは合わない。
  - ゲームで撮った画像は `game_O4_phase.png`(10・25・38F 黒い煙、43F 赤が点火、48・70・120F 溜まった後、145F 溜め攻撃)。
  - **プレイヤーが見えるように(11/26)**: 煙と炎を体のまわりの球から出すと、カメラ側の粒が体を隠した。カメラは後ろにあるので、**体の左右(X が -2.3〜-0.4 と 1.4〜3.3)だけ**に出すようにした(BodySmoke・BodyFlame を左右の2つのノードに分けた。PVA の Location の範囲で決める)。足の煙(LegSmokeF・B)は非表示。赤の主役は体の輪郭の光(ShellRed、a 190)。横や前からのカメラだと、片側の煙が体の前に来る。
- **S1「足元の煙の渦」に変えた(11/26)**。O4 は体が見えにくく、ユーザーが「あまりいやだ」と言ったため。案は `../../tools/ChargeHoldMock/charge_smoke_mock.html` の S1。ゲームのパスは `S1_Charging/Done/Charged.efk`(O4・A2 のファイルは残してある)。
  | ファイル | ノード | 中身 |
  |---|---|---|
  | `S1_Charging` | Swirl(5°/F で回る) / SwirlSmoke、Swirl2(10°/F) / SwirlSmoke(18F から) | 足元から煙が渦を描いて腰まで昇る。18F から内側の速い渦が足される |
  | `S1_Done` | RedFlash(足元) / RingSmoke / RingRed / TipStar | 煙の輪が地面を外へ広がる＋赤いふち＋足元の赤い光 |
  | `S1_Charged` | Swirl(3°/F) / SwirlSmoke / SwirlEmber、BladeGlow / BladeEmber | くるぶしの高さの低い渦＋赤い火の粉＋刀の赤い光 |
  - 渦の作り: 親(None)を Y 軸で回し(RotationValues の PVA の Velocity)、子の煙を Y 軸の円(Circle・AxisDirection YAxis)から出す。子は親についていくので、昇りながら回る。中心は親の位置(0.5, 0, 1.0)。
  - 外へ広がる輪: 円(Order・分割56)＋「生成角度に影響」(EffectsRotation)をオンにすると、PVA の速度の Z が外向きになる。
  - ゲームで撮った画像は `game_S1_phase.png`。煙は腰より下なので体は隠れない。
- **W1「寄る波紋 → 弾ける」に変えた(11/26)**。ユーザーが「煙ではなく足元の波動で」。案は `../../tools/ChargeHoldMock/charge_wave_mock.html` の W1。ゲームのパスは `W1_Charging/Done/Charged.efk`(S1・O4・A2 のファイルは残してある)。
  | ファイル | ノード | 中身 |
  |---|---|---|
  | `W1_Charging` | InRing(10F ごと) / InRingFast(18F から 5F ごと) / FootGlow | 地面の赤い輪が外から足元へ縮む(大きさ 16→1.5)。後半は間隔が詰まる。足元の光が 40F かけて灯る(FadeIn 40) |
  | `W1_Done` | OutRing / OutRingWhite / FootGlow | 赤い輪と白い輪が足元から外へ一気に広がる(SinglePVA で速く出て減速、最大 約26) |
  | `W1_Charged` | SlowRing(30F ごと) / FootGlow / BladeGlow / BladeEmber | 小さな赤い輪がゆっくり外へ広がり続ける＋足元の光＋刀の赤い光 |
  - 地面の輪: 画像 `CH_ShockRing` のスプライトを Billboard=Fixed・回転 X 90 で寝かせる。中心 (0.5, 0.1, 1.0)。DepthOffset 2。
  - ゲームで撮った画像は `game_W1_phase.png`。足元だけなので体は隠れない。
