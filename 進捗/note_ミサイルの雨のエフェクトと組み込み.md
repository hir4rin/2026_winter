# 【C++/DXライブラリ × Effekseer】ボスの「ミサイルの雨」のエフェクトを作ってゲームに組み込む

ボス（Kingmech）の攻撃「ミサイルの雨」の見た目を Effekseer で作り、ゲームに組み込みました。
設計は `note_ミサイルの雨の設計.md` に書いた形（ミサイルはエフェクトだけで、当たりは着弾の瞬間に決める）にそろえています。

この記事では、作ったエフェクトの中身と、ゲームへの組み込み方をまとめます。

---

## 1. 1発の流れ

ミサイル1発は、次の順に進みます。

```
撃つ ─── 予告(45F) ─── 落下(15F) ─── 爆発(4F) ─── 終わり
         赤い円          ミサイルが落ちる   爆発する        一覧から消す
```

段階ごとにエフェクトを1つずつ作りました。

| 段階 | ファイル | 見た目 |
|---|---|---|
| 予告 45F | `MissileWarning.efk` | 地面に赤い円。中心から橙の円が広がり、**外周に届いたら着弾**（残り時間が見て分かる）。外側で矢印つきの破線が回り、最後に3回点滅する |
| 落下 15F | `MissileFall.efk` | 上空（高さ1200）からミサイルが真下に落ち、**ちょうど15Fで地面に着く**。噴射の炎・光の尾・煙の柱つき |
| 爆発 | `MissileExplode.efk` | 閃光 → 火の玉 → 黒っぽい重い煙が上がって約1秒で消える。地面に衝撃波の輪・砂ぼこり・火の粉・焦げ跡 |

### 決めたこと
- **予告の円は「よけやすさ・見やすさ」**：赤ではっきり、残り時間が分かるように広がる円を入れた
- **爆発は「光と煙の重さ」**：明るい閃光と火の玉のあと、暗い煙をゆっくり上げる
- 色は「予告は赤、炎はオレンジ」

---

## 2. 大きさ

血しぶきと同じく、**1/20 の大きさで作って、ゲームでは20倍で再生**します。
20倍にすると、予告の円も爆発も**半径約200**です。

爆発の半径（`kExplodeRadius`）を変えるときは、3つとも同じ倍率に変えれば円と爆発がそろいます。

| 爆発の半径 | 再生の倍率 |
|---|---|
| 200 | 20倍 |
| 150 | 15倍 |
| 250 | 25倍 |

---

## 3. 素材

| 素材 | 作り方 |
|---|---|
| 予告の輪・塗り・破線、衝撃波の輪、焦げ跡、火の粉の筋（テクスチャ） | `tools/KingmechMissileFx/gen_fx_textures.ps1` で自作 |
| ミサイルのモデル（`Missile.efkmodel`） | `tools/KingmechMissileFx/gen_missile_model.ps1` で自作（先端が赤・黄色の帯・灰色の胴・尾翼4枚） |
| 煙・火・光（テクスチャ） | Effekseer 付属サンプルのコピー |

- スクリプトは PowerShell です。**BOM付き UTF-8 で保存しないと、日本語のコメントで壊れる**ので注意。
- 編集用の `.efkefc` も `data/Effect/Kingmech/` に置いてあります。

---

## 4. ゲームへの組み込み

### 全体の形
```
BossEnemyKingmech（ボス本体）
    ミサイルの一覧を持ち、毎フレーム Update する
        │ FireMissile(着弾位置)
        ▼
KingmechMissile（ミサイル1発分）
    段階(enum)・タイマーを持ち、段階が変わるときにエフェクトを出す
```

ミサイルの一覧をボスに持たせるのは、攻撃ステートが終わったあとも空中のミサイルを残すためです（設計メモの通り）。

### 変えたファイル

| ファイル | やったこと |
|---|---|
| `Main/System.h` / `System.cpp` | 3つのエフェクトを `AsyncData` に追加して読み込む・解放する |
| `Kingmech/Part/KingmechMissile.h` / `.cpp`（新規） | 1発分のクラス。段階が変わるときにエフェクトを20倍で再生。着弾で予告の円を止める |
| `Kingmech/BossEnemyKingmech.h` / `.cpp` | ミサイルの一覧と `FireMissile`。毎フレーム更新して、終わったものを消す。ボスが消えたら残りも消す |

### ミサイル1発のクラス（要点）
```cpp
void KingmechMissile::ChangePhase(Phase next)
{
	m_phase = next;
	m_timer = 0.0f;

	switch (m_phase)
	{
	case Phase::Falling:
		EffectManager::GetInstance().Play(AsyncData::KingmechMissileFallEffect, m_landPos, 0.0f, kEffectScale);
		break;
	case Phase::Explode:
		//着弾したら予告の円は消す
		StopEffekseer3DEffect(m_warningEffect);
		m_warningEffect = -1;
		EffectManager::GetInstance().Play(AsyncData::KingmechMissileExplodeEffect, m_landPos, 0.0f, kEffectScale);
		break;
	default:
		break;
	}
}
```

- 予告のエフェクトは、ミサイルを作った瞬間（コンストラクタ）に出す
- タイマーは**タイムスケールを掛けて進める**。エフェクトもタイムスケールで進むので、スローのときもずれない

### 確かめ方
- **M キー**で、プレイヤーの足元に1発落ちる（F キーは敵を出すのに使っているので M にした）

---

## 5. まだやっていないこと

- [ ] ゲームで見る（大きさ・暗さ、煙が多すぎて避けにくくないか、自作モデルが DxLib で出るか）
- [ ] 爆発の攻撃判定（`FixedAttackCol`）
- [ ] 攻撃ステート（`KingmechMissileRain`）で何発もまとめて撃つ
- [ ] 背中のパックから撃ち上げる発射のエフェクト（作るか未定）

6〜10発が続けて落ちると煙が重なって見えにくいかもしれません。そのときは煙を薄く・短くします。
