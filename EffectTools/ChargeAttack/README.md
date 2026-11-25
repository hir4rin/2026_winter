# 溜め攻撃(Y 長押し)のエフェクト

案は `../../tools/ChargeMock/charge_mock.html`。SEKIRO の不死斬りを参考に、A・B・C・E の4案を作った(D 紅い旋風は作っていない)。
**A と C に絞った**(11/25)。赤を強く・範囲を大きく・横からも見えるように作り直した(下の「A・C の作り直し」)。**そのあと A(赤黒の瘴気)に決定**。
ゲームで使うのは `A_Charge / A_Slash1 / A_Slash2.efkefc` を書き出した `2026_winter/data/Effect/ChargeAttack/Charge.efk / Slash1.efk / Slash2.efk`(名前を変えた)。B・C・E の .efkefc は記録として残してある(書き出したものはゲームのフォルダから消した)。

## A・C の作り直し(11/25)
- 書き換えは `edit_AC_v2.ps1`(`../../tools/EfkXml/` で .efkefc を XML にして書き換える)。元に戻すときは git の前の版から。
- **縦の帯**: 水平の輪は横から見ると線になるので、同じ角度で走る Ring を立てて足した(`InkWallSweep/Linger`・`RedWallSweep/Linger`)。外側の Location.y を負(上)、内側を正(下)にして、半径はほぼ同じ。色は上下のはしを透明にして真ん中を濃くした。
- 大きさ: 輪の外側の半径を A 1段目 8.5→10.6、2段目 9.5→11.9、C 1段目 6.6→8.8、2段目 7.2→9.6。煙・しずくの出る位置も同じ倍率。
- A: 輪の色を血の色寄りに、縁の赤を太く明るく、弧に沿って赤いもや(`Head/RedMist`)、火の粉を多く。溜めは刀の赤を強め、15F後から体のまわりに赤いもやが立ちのぼる(`AuraRise`)。
- C: 墨の下に朱の筆(`RedSweep/Linger`、外へはみ出す)、赤いしずく(`RedDrip`)、2段目の着地に朱の墨の輪(`ImpactRedInk`)と赤い飛沫(`ImpactDropsRed`)。溜めは円相の外に朱の円相(`EnsoRed`)・赤い飛沫・立ちのぼる煙(`EnsoRise`)・A と同じ刀の赤(`BladeGlow`)。
- ゲームで撮った画像: `game_A2.png` / `game_C2.png`(ステージ1、溜め → 1段目 → 2段目)。

## ファイル
案ごとに3つ。書き出しは `2026_winter/data/Effect/ChargeAttack/〇_〜.efk`(テクスチャは同じフォルダの `Texture/`)。

| 案 | 溜め(`〇_Charge`) | 1段目(`〇_Slash1`) | 2段目(`〇_Slash2`) |
|---|---|---|---|
| A 赤黒の瘴気 | 刀から黒い瘴気・赤い火の粉、15Fで刀に赤い光が走り切先が光る | 煙のむらの黒い輪(半径8.5)＋赤い縁、通り道に煙 | 同じ形で半径9.5・煙多め、着地で赤い十字の光 |
| B 一閃・遅れて割れる | 白銀の粒が刀へ集まる、15Fで切先に白い十字の光 | 細い白い線の輪 → 5F後に同じ弧が赤く裂けて火花 | 同じ形で大きく |
| C 墨の大筆 | 足元に墨の円相を15Fで描く、閉じたら墨が跳ねる | 筆のかすれの黒い輪、墨が垂れる | 同じ形＋着地で床に墨の輪と飛沫 |
| E 不死斬り×墨 | A の瘴気＋赤い縁の円相、15Fで刀に赤 | 筆の黒い輪＋赤い縁＋瘴気＋垂れ | 同じ形＋着地で墨の輪と赤い十字 |

ゲームで撮った画像: `game_A.png` / `game_B.png` / `game_C.png` / `game_E.png`(ステージ1、溜め → 1段目 → 2段目)。

## 作り
- 1/20 で作り、ゲームでは20倍。**エフェクトの +Z を前にして作り、ゲームでは `atan2(前.x, 前.z) + π` で回す**(π を足さないと前後が逆になった)。
- 溜め攻撃の2段はどちらも**横に回る斬り**(1段目は地上で約1回転、2段目は跳んで降りながら約1回転)。刀の先の通り道は DxLib で測った(`Combo_Attack_04_03` / `04_04`、刀の先の高さ約6、半径約5.5)。
- 弧は **Ring の Crescent**(X に90°回して水平)。角度は 0°=+Z(前)、増えると -X 側。`StartingAngle` / `EndingAngle` を Easing にして走らせる(`〇Sweep`、振っている間だけ) → 振り終わりに同じ角度で止まった輪(`〇Linger`)に入れ替えて消す。
  - 1段目: 3〜18F で -40°→390°(EaseOutQuadratic)。2段目: 16〜22F で 150°→500°(等速)。アニメの速さ0.8・段の始まりから数えたフレーム。
- 煙・しずく・火の粉は、弧の先と同じ向きに回る None(`Head`、Y の回転 = −弧の角度)の子に毎フレーム出す(`WhenCreating`)。
- 溜めは足元に置いて、`PlayerStateCharge::Update` で位置だけついていかせる。刀の位置は構えのループ(`Power_Attack_Loop`)で測った根元 (1.54,3.72,2.29)・先 (-0.97,2.18,0.20)。溜め完了の合図は15F目(`kChargeThreshold` と同じ)。離したら `StopEffekseer3DEffect`。
- 円相(C・E)は回復エフェクトと同じ描き方(AlphaTexture に描く順のランプ＋AlphaCutoff の FourPointInterpolation、0.99→0.4 を15F)。
- 床の物(円相・墨の輪)は `DepthOffset` 40 で床に埋もれないようにしている。**エディタではこの値がそのまま大きく効いて見えなくなる**(確かめるときは0にする)。
- 確認用の人型・目印は各ファイルの `Debug` ノード(A_Slash2・E_Slash1・E_Slash2 はばらばらに置いてある)。**書き出す前に消して、書き出したら開き直す**。
- 素材は `gen_charge_tex.ps1`(BOM 付き。白い画像で形はアルファ、色はノードで付ける)。

## ゲーム側
- `AsyncData::ChargeChargeEffect / ChargeSlash1Effect / ChargeSlash2Effect`(`System::LoadCommon` で読む)。案を切り替える F11 は A に決めたときに消した。
- 再生: `PlayerStateCharge::Enter`(溜め)、`PlayerStateChargeAttack::StartPhase`(1段目・2段目)。
