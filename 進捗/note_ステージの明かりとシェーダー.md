# ステージの明かり(街灯・縦穴の明かり)とシェーダーのまとめ

> **(11/02 追記)** このメモは「重ね描き」の時点のもの。床がちかちか・後ろが透けたので、今は**1回で描く `LitModelPS.hlsl`** になっている。最新は `note_11月1-2日の作業まとめ.html`。

2026-11-01〜02 にやったことのまとめ。**ゲーム側のコード**が中心。最後に Unity 側と壁走りの修正も短く書いた。

---

## 1. 全体の流れ

```
[Unity] 明かりを置く(StageLight / 街灯 / 床の誘導灯)
   │  書き出し(CyberStageTool.Export)
   ├─→ 見た目の mv1 … 電球・床の光のたまり(半透明の板)が入る ← これは「光って見える絵」
   └─→ data/Stage/Lights/Stage005.csv … 明かりの位置・色・届く距離
                │
[ゲーム] Stage::GameInit で StageLights が CSV を読む
                │ 毎フレーム
        StageLights::Update(プレイヤーの位置)
           └ 近い順に最大16灯を選んで ModelShaderManager に渡す(定数バッファ)
                │ 描画のとき
        いつも通り描く(MV1DrawModel)
           └ その上に「明かりの光だけ」を加算でもう1回描く(DrawStageLight + StageLightPS.hlsl)
```

- 床の光のたまり(板)は**絵**。キャラは照らされない。
- シェーダーで足す光は**本当に照らす光**。床・壁・キャラ・敵に色が乗る。
- 両方あるので「遠くからは板の光で道が分かる」「近くでは本当に照らされる」になる。

---

## 2. 変更したファイル(ゲーム側)

| ファイル | 新規/変更 | 中身 |
|---|---|---|
| `Render/Shader/StageLightPS.hlsl` | 新規 | 明かりの光だけを出すピクセルシェーダー(このメモの中心) |
| `Stage/StageLights.h/.cpp` | 新規 | 明かりの CSV を読み、プレイヤーに近いものを選んでシェーダーに渡す |
| `Render/ModelShaderManager.h/.cpp` | 変更 | 明かり用シェーダーの読み込み、定数バッファ、`SetStageLights` / `DrawStageLight` を追加。`DrawModel` の後に光を足すように |
| `Stage/Stage.h/.cpp` | 変更 | `StageLights m_lights` を持つ。GameInit で読む、Draw で光を足す |
| `Scene/SceneMain.cpp` | 変更 | 毎フレーム `m_stage->GetLights().Update(プレイヤーの位置)`、F2 で「Stage Lights」ウィンドウ |
| `Character/Player/Player.cpp`・`PlayerWeapon.cpp` | 変更 | `MV1DrawModel` → `ModelShaderManager::DrawModel`(プレイヤーと刀も照らされる) |
| `2026_winter.vcxproj` / `.filters` | 変更 | 上の新しいファイルを登録(hlsl は None) |

敵・敵の武器・部位・ガトリングは、もともと `ModelShaderManager::DrawModel` で描いているので、何も変えずに照らされる。

---

## 3. シェーダー入門(このプロジェクトに必要な分だけ)

### 3-1. シェーダーとは
画面にモデルを描くとき、GPU は2つの小さなプログラムを動かしている。

| 名前 | いつ動く | 何をする |
|---|---|---|
| **頂点シェーダー (VS)** | モデルの**頂点ごと**に1回 | 頂点の位置を「画面のどこか」に変換する。ついでに法線・テクスチャ座標・ワールド座標などを次に渡す |
| **ピクセルシェーダー (PS)** | 画面の**ピクセルごと**に1回 | そのピクセルの**色**を決める |

VS が出した値は、三角形の中でなめらかに混ぜられて(補間されて)PS に届く。
だから PS では「このピクセルのワールド座標」「このピクセルの面の向き(法線)」が分かる。

DxLib はふだん自分で用意したシェーダー(標準シェーダー)で描いている。
`MV1SetUseOrigShader(TRUE)` ＋ `SetUseVertexShader` / `SetUsePixelShader` で、**自作のシェーダーに差し替えられる**。

### 3-2. このプロジェクトに前からあったもの
- `MV1VertexShader.hlsl`(頂点シェーダー)… 10/08 に入れたもの。血殺中に敵を黒くするのに使っていた。
  出力(`VS_OUTPUT`)に **WorldPosition(ワールド座標)と WorldNormal(ワールドの法線)** が入っているので、明かりの計算にそのまま使えた。
- 頂点シェーダーは「モデルの頂点の種類」ごとに別々にコンパイルしてある(骨1本/4本/8本、ノーマルマップあり/なし)。`ModelShaderManager` が選ぶ。

### 3-3. なぜ自作シェーダーが必要だったか(最初の案Aが駄目だった理由)
最初は DxLib の標準のライト(`CreatePointLightHandle`)で試した。ほぼ効かなかった。理由は2つ。

1. **材質の拡散が 0.1**
   キャラ・ステージの mv1 は「拡散 0.1・環境光 0.9」にしてある(ライトの向きで明るさが変わらないように)。
   ライトの光は「ライトの色 × 材質の拡散」なので、1割しか効かない。
2. **DxLib が光の合計を 1 で頭打ちにする**
   ライトを強くしても「合計1 × 0.1」で止まる。色も白っぽく飛ぶ。

→ そこで**材質の拡散を使わず**、「テクスチャの色 × 明かりの色」で光を作る自作のピクセルシェーダーにした。

### 3-4. 「もう1回、加算で描く」というやり方
自作シェーダーで**全部**(DxLib のライト・フォグ・影…)を作り直すのは大変で、見た目も変わってしまう。
そこで:

1. まず**いつも通り** DxLib で描く(今までの見た目そのまま)。
2. 同じモデルを**明かり用のシェーダーでもう1回描く**。このシェーダーは「明かりの光の分だけ」の色を出す。
3. 2回目は**加算**(今ある色に足す)で描く。

```
画面の色 = いつもの色 + 明かりの光
```

明かりから遠い所は「光 = 0(黒)」なので、足しても何も変わらない。明かりの近くだけ明るく・色付きになる。

### 3-5. StageLightPS.hlsl を上から読む

#### (1) テクスチャを受け取る
```hlsl
Texture2D g_DiffuseMapTexture : register(t0);
SamplerState g_DiffuseMapSampler : register(s0);
```
DxLib はモデルの描画のとき、マテリアルのテクスチャを **t0 / s0** に自動でセットしてくれる。
`register(t0)` は「0番のテクスチャの置き場所」という意味。

#### (2) 頂点シェーダーからの入力
```hlsl
struct PS_INPUT { float4 Diffuse : COLOR0; ... float3 WorldPosition : Position1; float3 WorldNormal : Normal1; };
```
- **頂点シェーダーの出力(VS_OUTPUT)と、並びを全く同じにする**必要がある(D3D11 の決まり。ずれると値が入れ替わって変な色になる)。
- `: COLOR0` などは「この値の名札(セマンティクス)」。
- ノーマルマップのある頂点タイプ(BUMPMAP)は、VS_OUTPUT の途中に接線(VTan / VBin)が入って並びが変わる。
  だから `#ifdef BUMPMAP` を入れて、**BUMPMAP 付きと無しの2つをコンパイル**している(`m_lightPixelShader` / `m_lightBumpPixelShader`)。
  ※ 黒くするシェーダーは先頭の3つしか使わないので、この問題が無かった。

#### (3) 明かりの情報を受け取る(定数バッファ)
```hlsl
#define LIGHT_MAX 16
cbuffer StageLightBuffer : register(b4)
{
	float4 g_LightPosRange[LIGHT_MAX];	// xyz:位置 w:届く距離
	float4 g_LightColor[LIGHT_MAX];		// rgb:色×強さ
	float4 g_Param;						// x:数 y:フォグの始まり z:フォグの終わり w:裏側への回り込み
};
```
- **定数バッファ** = C++ からシェーダーへ「全ピクセル共通の値」を渡す箱。毎フレーム1回書き換える。
- `register(b4)` は4番の箱。0〜3番は DxLib が自分で使っているので、空いている4番にした。
- C++ 側(`ModelShaderManager::StageLightConstant`)と**並び・大きさを同じに**しないといけない。
- 全部 `float4`(4個組)にしているのは、定数バッファが16バイト(float 4個)単位で並ぶ決まりがあるから。
  3個で足りる所も4個にして、空いた所に別の値を詰めている(位置xyz＋距離w、など)。

#### (4) main の中身
```hlsl
float4 texColor = g_DiffuseMapTexture.Sample(g_DiffuseMapSampler, input.TexCoords0_1.xy);
float alpha = texColor.a * input.Diffuse.a;
clip(alpha - 0.01f);
```
- このピクセルのテクスチャの色を取る(`Sample`)。
- 透明な所(アルファがほぼ0)は `clip` で**描かない**(髪の毛や金網の抜けている所に光が乗らないように)。

```hlsl
float3 normal = normalize(input.WorldNormal);
```
- 面の向き(法線)。補間で長さが1でなくなるので、`normalize` で長さ1に直す。

明かりごとのループ:
```hlsl
float3 toLight = g_LightPosRange[i].xyz - input.WorldPosition;  // このピクセルから明かりへの向きと距離
float dist = length(toLight);
float range = g_LightPosRange[i].w;
float atten = saturate(1.0f - dist / range);   // 近いほど1、届く距離で0
atten *= atten;                                // 2乗して、なめらかに消える形に
```
- `saturate(x)` は「0〜1に収める」。
- 例: 届く距離 720(=7.2m)の明かりで
  - 距離 0 → (1−0)² = **1.0**
  - 距離 360(半分) → (1−0.5)² = **0.25**
  - 距離 720 → **0**(ちょうど消える。DxLib のライトのように円の縁がくっきり出ない)

```hlsl
float ndl = dot(normal, toLight / max(dist, 0.001f));
ndl = saturate(ndl * (1.0f - g_Param.w) + g_Param.w);
```
- `dot(面の向き, 明かりへの向き)` = 面が明かりの方を向いているほど1、横向きで0、背を向けると負。
- そのままだと明かりに背を向けた面が真っ黒になって不自然なので、**裏側も w(0.2)だけ照らす**(wrap ライティング)。
  `ndl × 0.8 + 0.2` → 真正面 1.0、横 0.2、裏 0(負は saturate で0)。

```hlsl
light += g_LightColor[i].rgb * atten * ndl;
```
- 明かりの色 × 距離の減り方 × 面の向き、を全部の明かりで足していく。

```hlsl
float fogFade = saturate((g_Param.z - input.VPosition.z) / max(g_Param.z - g_Param.y, 1.0f));
return float4(texColor.rgb * light * fogFade, alpha);
```
- `VPosition.z` はカメラからの奥行き。ステージ1は 15m〜90m で夜のフォグがかかるので、光もその間で1→0に薄くする(遠くで光だけ浮かないように)。
- 最後に **テクスチャの色 × 光**。赤レンガにオレンジの光なら赤っぽいオレンジ、灰色の床に水色なら水色っぽい灰、になる。
  (材質の拡散 0.1 を使っていないので、ちゃんと色が出る)

### 3-6. 2回目の描画の注意点(ModelShaderManager::DrawStageLight)
1. **加算にする方法**: `SetDrawBlendMode` は**モデルの描画には効かなかった**(全部上書きになり、光以外が真っ黒になった)。
   なので**マテリアルごとに** `MV1SetMaterialDrawBlendMode(ADD)` にして描き、描き終わったら元の値に戻す。
2. **Zバッファに書かない**: `SetWriteZBuffer3D(FALSE)`。1回目と全く同じ形・同じ深さに重ねるだけなので、比べるだけで書かない。
   (頂点シェーダーが DxLib の標準と同じ計算なので、深さがずれてちらつくことは無かった)
3. **メッシュごとに頂点シェーダーを選ぶ**: 1つのモデルに骨4本と8本のメッシュが混ざっていることがあるので、`MV1DrawMesh` でメッシュごとに描く。
   頂点シェーダーを用意していない頂点タイプ(FREE_FRAME = 骨9本以上)のメッシュは、光を足さずに飛ばす。
4. 終わったら `SetUseVertexShader(-1)` などで**元に戻す**(ほかの描画に影響させないため)。

---

## 4. C++ 側の流れ

### StageLights(`Stage/StageLights.h/.cpp`)
- `Load(番号, ずらし)`: `data/Stage/Lights/Stage%03d.csv`(番号は敵配置と同じ)を読む。ファイルが無いステージは何もしない。
  ステージのモデルを床の高さに合わせてずらしているので、その分を明かりの位置にも足す。
- `SetFogRange(始まり, 終わり)`: ナイトフォグのステージ(ステージ1)だけ、フォグの距離を渡す。
- `Update(プレイヤーの位置)`:
  1. プレイヤーから `m_activeDistance`(40m)以内の明かりを集めて、近い順に並べる。
  2. 上から16個まで。遠い端 `m_fadeDistance`(10m)では強さを少しずつ下げる(入れ替わりでパッと消えないように)。
  3. 色 = CSV の色 × 強さ(intensity) × `m_colorScale`(3) × 遠い端の減り具合。
  4. `ModelShaderManager::SetStageLights` に渡す。
- `DrawDebugWindow()`: F2 で出る「Stage Lights」ウィンドウ(下の表)。

### ModelShaderManager(`Render/ModelShaderManager.h/.cpp`)
- `Init()`: 明かり用ピクセルシェーダーを2つ(BUMPMAP 有り/無し)コンパイル、定数バッファを作る。
- `SetStageLights(明かり, フォグ, wrap)`: 定数バッファに書いて `UpdateShaderConstantBuffer`。明かりの数を覚える。
- `DrawStageLight(モデル)`: 上の 3-6。明かりが0なら何もしない。
- `DrawModel(モデル)`: 黒くするモード(血殺中の敵)でなければ、`MV1DrawModel` → `DrawStageLight`。

### 1フレームの順番
```
SceneMain::NormalUpdate
  … プレイヤー・敵・当たり判定・BattleManager の更新
  m_stage->GetLights().Update(プレイヤーの位置)   ← ここで今フレームの明かりが決まる
SceneMain::NormalDraw
  敵(DrawModel → 光を足す。血殺中は黒いので足さない)
  ステージ(Stage::Draw → MV1DrawModel → DrawStageLight)
  プレイヤー・刀(DrawModel → 光を足す)
```

---

## 5. 調整のつまみ

| 何を | どこで | 今の値 |
|---|---|---|
| 光の強さ全体 | F2「Stage Lights」の Color Scale / `StageLights.h` の `m_colorScale` | 3(6だと強すぎた) |
| 使う明かりの範囲 | Active Distance / `m_activeDistance` | 4000(40m) |
| 遠い端で薄くする幅 | Fade Distance / `m_fadeDistance` | 1000(10m) |
| 背を向けた面の明るさ | Wrap / `m_wrap` | 0.2 |
| オン/オフ | Enable | オン |
| 同時に使う明かりの数 | `ModelShaderManager::kStageLightMax` と `StageLightPS.hlsl` の `LIGHT_MAX`(**2つ同時に変える**) | 16 |
| 明かりごとの色・強さ・届く距離 | Unity(明かりの Light コンポーネント)→ 書き出し → CSV | 暖色・水色・淡い白(戦う場所) |

---

## 6. 気をつけること・限界
- **重さ**: 照らされるモデルを2回描いている。ステージ(約47万三角形)も2回。重ければ「明かりの近くのメッシュだけ2回目を描く」などで減らせる。
- **影は出ない**: 明かりは壁を通り抜けて向こう側も照らす(距離だけで決まる)。
- **シェーダーは実行時にコンパイル**: `Render/Shader/StageLightPS.hlsl` をゲームの起動フォルダからの相対パスで読む。書き間違えると `ModelShaderManager::Init` の assert で止まる(VS の出力ウィンドウにエラーが出る)。
- **16灯を超えると遠いものから使われない**: プレイヤーの近くで16灯を超えるほど密に置かないこと。
- 血殺のフォグ(`MV1SetDifColorScale`)は1回目の描画にだけ効く。2回目(光)はテクスチャの色だけ使うので影響しない。
- 明かりの無いステージ(CSV が無い)では、2回目の描画をしないので今まで通り。

---

## 7. ついでにやったこと

### Unity 側(`ninja_motion/Assets/Editor/CyberStageTool.cs`・`CyberStagePalette.cs`)
- `StageLight`(明かり: 床の光のたまり＋Unity のライト。電球の箱は付けないこともできる)、`FloorGuide`(床の誘導灯)、`StreetLamp`(アセットの街灯＋頭に明かり。明かりは街灯の子)。
- 書き出しで `data/Stage/Lights/Stage005.csv` を出す(`ExportLights`。名前が `StageLight_` で始まるライトだけ。アセットに元から入っているライトは入れない)。
- 色: Warm(オレンジ=進む道)、Cool(水色=壁走りの入口)、Red、Dim(淡い白・弱め=戦う場所の街灯)。
- 当たり判定を見た目の物の子にした(見た目を動かすと当たり判定も付いてくる)。
- パレットに「明かり(導線)」「床の誘導灯」「街灯(明かり付き)」。

### 壁走りの連続(`PlayerStateWallRunKick.cpp/.h`・`PlayerStateFall.cpp`)
- 壁走りから蹴った後に次の壁へ張り付く条件が「アニメのフレーム ≥ 23」だったが、そのアニメは 21.6 で止まるので**一度も張り付けなかった**。
  → 状態に入ってからのフレーム数で数え、**6フレーム後から張り付ける**ようにした(`kWallRunRegrabFrame`)。
- 落下中(Fall)も、壁走りゾーンの壁に当たってスティックを倒していれば壁走りへ移るようにした(蹴った勢いの上昇は10フレームで終わり、届くのは落下中なため)。
