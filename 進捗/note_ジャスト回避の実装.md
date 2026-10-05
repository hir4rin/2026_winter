# 【C++/DXライブラリ】アクションゲームにジャスト回避を実装した話

自作の3Dアクションゲーム（C++ / DXライブラリ）に「ジャスト回避」を実装しました。
敵の攻撃をギリギリで避けるとスローになる、よくあるあれです。

この記事では、どういう設計にしたか、そしてなぜその形にしたのかをまとめます。

## 前提：今の当たり判定の仕組み

まず、ゲームの当たり判定はこういう構成になっています。

- **Collider**：全ての当たり判定の基底クラス。陣営（Player / Enemy など）と役割（Hit / Attack など）のタグを持つ
- **HitCol**：やられ判定。攻撃が当たったら持ち主の `OnDamage` を呼ぶ
- **AttackCol**：攻撃判定。当たった相手の ID をリストに記録して、同じ相手に2回当たらないようにしている
- **CollisionManager**：登録された判定同士の当たりを毎フレーム計算して、当たったら両方の `OnCollision` を呼ぶ

プレイヤーの行動はステートパターンで管理していて、回避は `PlayerStateDodge` が担当しています。

## 最初の案：ジャスト回避用の判定を OnDamage で受け取る

最初に考えたのは、「プレイヤーにジャスト回避用の判定を持たせて、それに `OnDamage` が来たらジャスト回避にする」という方法です。

大きめの判定で「攻撃がかすめた」ことを拾うので、回避で攻撃範囲の外に出てしまっても成立します。この方向性自体は良かったのですが、今の仕組みにそのまま載せると3つ問題がありました。

### 問題1：どちらの判定に当たったか区別できない

敵の攻撃判定は、役割が `Hit` の相手には誰にでも `OnDamage` を呼びます。ジャスト回避判定も `Hit` にすると、普通のやられ判定と両方から `Player::OnDamage` が呼ばれてしまいます。
しかも `OnDamage` に渡されるのは攻撃側の判定なので、プレイヤー側からは「自分のどの判定に当たったか」が分かりません。

### 問題2：当たった ID の記録が先に埋まる

攻撃判定は、同じ相手には1回しか当たらないように ID を記録しています。
ジャスト回避判定を常に出しっぱなしにすると、回避ボタンを押す前に攻撃が触れた時点で ID が記録されてしまいます。そうなると、そのあと回避してももう通知が来ません。

### 問題3：同じフレームで両方に当たったときの順番

ジャスト回避判定と通常のやられ判定に同じフレームで当たると、処理の順番しだいで「ダメージを受けたうえでジャスト回避も成立する」ということが起こり得ます。

## 最終的な設計

この3つを解決するために、こういう形にしました。

- ジャスト回避専用の判定クラス **JustDodgeCol** を作る（役割も `JustDodge` を新しく追加）
- その判定は **PlayerStateDodge が回避中だけ作る**
- 受付時間中は **無敵** にする

役割の分担はこうです。

- **CollisionManager**：当たったかどうかの計算（今まで通り）
- **JustDodgeCol**：当たったことを Player に伝えるだけ
- **PlayerStateDodge**：受付時間の管理と、成功したときの処理

### なぜステートに判定を持たせたか

実は、攻撃ステート（PlayerStateAttack）がすでに同じことをしていました。`Enter` で攻撃判定を作って、`Exit` で消しています。これに揃えると、作りが統一されて読みやすくなります。

さらに、こういうメリットがあります。

- 回避中にしか判定が存在しないので、**問題2が起きない**
- 回避のたびに作り直すので**毎回新しい ID になる**。長く続く攻撃に2回続けて回避しても、2回とも判定できる
- 被弾などで回避が途中で終わっても `Exit` は必ず通るので、判定が残らない

ちなみに「当たったかどうかの計算までステートの中で自前でやる」という案も考えましたが、やめました。形ごとの当たり計算を CollisionManager とは別にもう一度書くことになり、判定のサイズを変えたときに2か所の計算がずれやすいからです。

## 実装

### 1. 役割を追加する

```cpp
enum class ColRole
{
    None = -1,
    Hit,//当たり判定
    Attack,//攻撃判定
    UltAttack,//必殺技判定
    WallKickZone,
    WallRunZone,
    JustDodge,//ジャスト回避判定(回避中だけ存在する)
};
```

### 2. JustDodgeCol を作る

HitCol とほぼ同じ形です。持ち主の位置についていって、攻撃が当たったら持ち主に伝えるだけです。

```cpp
void JustDodgeCol::OnJustDodgeInterFace(Collider& other, CharacterBase::AttackData& data)
{
    //所有者に通知する
    auto owner = m_owner.lock();
    if (!owner)return;

    owner->OnJustDodge(other, data);
}
```

持ち主はステートではなく Player にしました。ステートは切り替わるたびに作り直されるので、Player を経由する方が安全だからです。

### 3. 敵の攻撃側に分岐を追加する

敵の攻撃判定が当たったときの処理に、ジャスト回避判定用の分岐を足しました。同じ攻撃で2回成功しないように、既存の ID の記録をそのまま使っています。これで**問題1が解決**します。

```cpp
//プレイヤーのジャスト回避判定に当たったとき
else if (other.GetTag().role == Collider::ColRole::JustDodge)
{
    int otherId = other.GetId();
    auto it = std::find(m_hitIds.begin(), m_hitIds.end(), otherId);
    if (it == m_hitIds.end())
    {
        m_hitIds.push_back(otherId);
        auto justDodgeCol = dynamic_cast<JustDodgeCol*>(&other);
        if (justDodgeCol)
        {
            justDodgeCol->OnJustDodgeInterFace(*this, *m_attackData);
        }
    }
}
```

### 4. ステートの基底クラスに受け口を作る

全部のステートに、何もしない仮想関数を2つ追加しました。中身を書くのは回避ステートだけです。

```cpp
//ジャスト回避判定に敵の攻撃が当たった時の処理//回避状態でオーバーライドする
virtual void OnJustDodge(Collider& other, const CharacterBase::AttackData& data) {};
//無敵かどうか//trueの間はダメージを受けない
virtual bool IsInvincible()const { return false; }
```

Player は、通知を今のステートにそのまま渡します。

```cpp
void Player::OnJustDodge(Collider& other, AttackData& data)
{
    if (!m_currentState)return;
    m_currentState->OnJustDodge(other, data);
}
```

`OnDamage` の最初では無敵かどうかを確認します。これで**問題3が解決**します。

```cpp
//状態による無敵(ジャスト回避の受付中など)はダメージを受けない
if (m_currentState && m_currentState->IsInvincible())return;
```

### 5. 回避ステートで判定を作る・消す

`Enter` で判定を作ります。

```cpp
m_justDodgeTimer = 0.0f;
m_isJustDodged = false;
m_justDodgeCol = std::make_shared<JustDodgeCol>(m_owner);
m_justDodgeCol->ColInit({
    .pos = player->m_rb.m_pos,
    .offset = Vector3(0, kPlayerCenter, 0),
    .shape = std::make_unique<SphereShape>(kJustDodgeColRadius),
    .tag = {Collider::Faction::Player, Collider::ColRole::JustDodge},
    .isActive = true,
    .isTrigger = true
    });//押し戻しはしない
```

`Update` で受付時間を数えて、過ぎたら判定を無効にします。

```cpp
m_justDodgeTimer += 1.0f * System::GetInstance().GetTimeScale();
if (m_justDodgeCol && m_justDodgeTimer > kJustDodgeFrame)
{
    m_justDodgeCol->SetIsActive(false);
}
```

成功したときはスローにします。1回の回避で1回だけ成功するようにフラグで管理しています。

```cpp
void PlayerStateDodge::OnJustDodge(Collider& other, const CharacterBase::AttackData& data)
{
    if (m_justDodgeTimer > kJustDodgeFrame)return;
    if (m_isJustDodged)return;
    m_isJustDodged = true;

    //スロー演出
    System::GetInstance().SetTimeScaleForFrames(kJustDodgeTimeScale, kJustDodgeSlowFrame);
}
```

`Exit` では判定を必ず消します。

```cpp
void PlayerStateDodge::Exit()
{
    //被弾などで途中で状態が変わっても必ず消す
    ReleaseJustDodgeCol();
}
```

### 調整する値

数値はファイルの上にまとめて、あとから調整しやすくしました。

- 判定の半径：120（やられ判定の50より大きくして、かすった攻撃も拾う）
- 受付フレーム数：10
- 成功時の時間の速さ：0.2倍
- スローの長さ：30フレーム

## ハマったところ：文字コード

新しく作ったファイルを BOM なしの UTF-8 で保存したら、Visual Studio が Shift-JIS として読んでしまい、日本語コメントのせいでビルドエラーになりました。
エラーの内容は構文エラーなのに、コード自体は合っているという分かりにくいパターンでした。他のファイルに合わせて BOM 付き UTF-8 にしたら直りました。

## 今後やること

- 実際にプレイして、半径と受付フレーム数を調整する
- 今は回避の最初の10フレームが必ず無敵になるので、それで良いか遊んで確かめる
- スローがゲーム全体にかかってプレイヤーも遅くなるので、プレイヤーだけ通常速度にするか検討する
- 成功したあとに反撃につなげる

## まとめ

- ジャスト回避は「専用の判定 + 回避中だけ作る」形にした
- 判定を回避ステートに持たせることで、受付時間の管理と後片付けがステートの中で完結した
- 「当たったかの計算」は既存の仕組みに任せて、ステートは「いつ受け付けるか」と「成功したら何をするか」だけを担当する

最初は `OnDamage` を使い回そうとしていましたが、ダメージとジャスト回避の通知を分けたことで、それぞれの処理がシンプルになりました。
