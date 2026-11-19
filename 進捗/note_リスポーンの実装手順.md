# リスポーンの実装手順

死亡の条件(落下死)はもう入っている(2026-11-19)。ここに書くのは、死んだあとにリスポーンさせる部分で、自分で実装する。

## もう入っているもの

- `Player::GetDeathCause()` で死んだ原因が分かる: `None` / `Hp`(HPが0) / `Fall`(落下)
- 落下死になる条件(`Player::CheckFallDeath`、`Player::Update` の中で毎フレーム調べる)
  - 壁ゾーンの `DeathZone` に入った(壁ゾーンのエディタで種類に `DeathZone` を選んで置く。線は赤)
  - `Stage::GetKillY()` より下に落ちた(当たり判定のモデルの一番下から 500 下。テスト -600 / デモ -1800 / 列車 -670 / ステージ1 -1900)
- 落下死すると `m_isDead = true` になり、`Player::Update` はそこで止まる(ステートもアニメも動かない。その場で止まる)
- HP が 0 のときは今まで通り `PlayerStateDie` で、アニメが終わると `m_isDead = true`
- **今はどちらも死んだまま何も起きない**。下の手順で戻す

## 方針(会話で決めたこと)

| 死に方 | 戻す場所 | 敵 |
|---|---|---|
| 落下 | 最後に安全に着地した場所 | そのまま(フェーズはやり直さない) |
| HP 0 | 今のフェーズが始まったときの場所(チェックポイント) | 今のフェーズをやり直す |

- 進行方向の案内の点とは分ける(別のデータ)
- 落下も死亡回数(`ResultData::deathCount`)に数えるかは決めていない。数えないなら落下のときだけ +1 しない

## 手順

### 1. 最後に安全に着地した場所を覚える(Player)

- `Player` に `Vector3 m_lastSafePos` を足す
- `Player::Update` の最後のほう(ステートの更新の後)で、次を全部満たしたら `m_lastSafePos = m_rb.m_pos`
  - `IsFloor()`(または `WasFloor()`)が true
  - 死んでいない(`m_deathCause == DeathCause::None`)
  - DeathZone の中でない
- 足場の端で覚えると、戻ったときにまた落ちる。気になったら「数フレーム続けて着地している」ときだけ覚える、または `CollisionManager::HasFloorAt` のように周りに床があるか見る
- 最初の値はスタート地点(`Init` で入れる)

### 2. チェックポイントを覚える(PhaseManager)

- `PhaseManager` に `Vector3 m_checkpointPos`
- `ChangePhase` の中で、`next > 0` のときに `m_checkpointPos = player->GetLastSafePos()`(1 で作る getter)
- `Init` の Start の行の前にもスタート地点を入れておく

### 3. フェーズのやり直し(PhaseManager / EnemyManager)

- `EnemyManager` に、敵・死体を全部消す関数(`DebugClearEnemies` と同じ中身。名前を `ClearEnemies` などにして、デバッグからもそれを呼ぶ)
- `EnemyManager` に撃破数を戻す setter(`m_totalKillCount` を入れる)
- `PhaseManager::RetryPhase()`:
  ```cpp
  enemyMgr->ClearEnemies();
  enemyMgr->SetTotalKillCount(m_killCountAtPhaseStart);
  m_phaseFrame = 0;
  if (m_currentPhase > 0)enemyMgr->StartPhase(m_currentPhase);
  ```
  - `ChangePhase` を呼ばないこと(チュートリアルなどのイベントがもう一度動く)
- 攻撃権: 消した敵の分は `AttackTokenManager::Update` が回収するはず。すぐ攻撃が来ないなら、リセットする関数を足す
- 前のフェーズの生き残りも消える(グループの AllDead で進んだ場合)。気になったら、`ChangePhase` のときに生きている敵の配置データを覚えて出し直す

### 4. プレイヤーを戻す(Player)

`Player::Respawn(const Vector3& pos)` を作る。

- `SetPos(pos)`、`m_rb.ResetVel()`、`m_rb.m_moveVel = 0`
- `m_hp = GetMaxHp()`、`m_isDead = false`、`m_deathCause = DeathCause::None`
- ステートを Idle にする(`ForceIdleState` があればそれ)
- 無敵時間を入れる(`m_damageInfo.damageTimer` を長めに)
- ロックオン・暗殺対象・確殺対象・ボスのトドメの対象などを外す

### 5. 死んでから戻るまでの流れ(SceneMain)

`SceneMain::NormalUpdate` の頭で見る(敵の Update の途中で敵を消さないため)。

1. `player->GetIsDead()` が true になったら暗転を始める(フェード)
2. 真っ黒になったら
   - 原因が `Hp` なら `phaseMgr->RetryPhase()`、戻す場所は `phaseMgr->GetCheckpointPos()`
   - 原因が `Fall` なら戻す場所は `player->GetLastSafePos()`
   - `player->Respawn(場所)`
   - カメラをプレイヤーの後ろにつける(`CameraManager` の PlayerFollowCamera へ、ブレンドなし)
   - 再生中のエフェクトを止める、`deathCount` を +1(`ResultRecorder`)
3. 明転する

- リザルトへのフェードアウト(`GetIsBattleEnd`)と重ならないようにする(死亡中はクリアにしない、など)
- ボス戦の視野角(`SetFovOffset`)はカメラをリセットしても消さない

### 6. 確認

- 壁ゾーンのエディタで DeathZone を置いて入る → 落下死 → 着地した場所に戻る
- 穴から下へ落ちる → `GetKillY` より下で落下死 → 戻る
- 敵にやられる → フェーズの最初の場所に戻り、敵が出し直される・撃破数が戻る
- ボス戦でやられる → ボスの HP が戻る
- F4 でボスへ飛んだ直後にやられたとき(チェックポイントがエリアの手前になるか)
