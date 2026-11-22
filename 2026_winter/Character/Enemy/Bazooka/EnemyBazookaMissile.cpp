#include "EnemyBazookaMissile.h"
#include "EnemyBazookaMissileCol.h"
#include "Main/System.h"
#include "Render/EffectManager.h"
#include "EffekseerForDXLib.h"
#include "Collision/FixedAttackCol.h"
#include "Collision/SphereShape.h"
#include "DxLib.h"
#include <cmath>
#include <algorithm>

//=============================================================================
// ミサイルを作る順番(TODO の番号の順に進める)
//
//  手順1  コンストラクタ: 距離を出す                      … このファイル
//  手順2  Update / UpdateFly: 割合でまっすぐ進める         … このファイル
//  手順3  Draw: 仮の見た目                                 … このファイル
//  手順4  EnemyManager で持つ・回す                        … EnemyManager.cpp
//  手順5  EnemyBazooka で撃つ                              … EnemyBazooka.cpp
//         ここで一度ゲームで見る(銃口から着弾点へまっすぐ飛べばOK)
//  手順6  CalcFlyPos: sin で横と縦にずらす                 … このファイル
//  手順7  Init: 着弾点に予告を出す                         … このファイル
//  手順8  Explode / CreateExplodeCol: 爆発する             … このファイル
//  手順9  飛んでいる間の判定(触れたら爆発)               … EnemyBazookaMissileCol.cpp
//  手順10 ステージに当たったら爆発                         … このファイルの UpdateFly
//
// 1回に1つだけ足して、ゲームで見てから次へ進むのがおすすめ。
// 一度に全部書くと、動かなかったときにどこが悪いか分からなくなるため。
//=============================================================================

namespace
{
	constexpr float kSpeed = 30.0f;//1Fで進む距離

	//sinの揺れ//幅は距離に比例させて、上限で止める
	constexpr float kSideWidthRate = 0.2f;//横の幅(距離に対する割合)
	constexpr float kMaxSideWidth = 400.0f;//横の幅の上限
	constexpr float kUpWidthRate = 0.15f;//縦の幅(距離に対する割合)
	constexpr float kMaxUpWidth = 300.0f;//縦の幅の上限
	constexpr float kSideWaveNum = 1.0f;//横の山の数//整数にすると着弾点にぴったり落ちる
	constexpr float kUpWaveNum = 1.0f;//縦の山の数//1なら上に弧を描く
	constexpr float kMinWidthRate = 0.7f;//幅の乱数の下限(1.0までのどこか)

	constexpr float kFlyColRadius = 40.0f;//飛んでいる間の判定の半径//1Fで進む距離より大きくしてすり抜けを防ぐ

	//予告・爆発はKingmechのミサイルのエフェクトを使う
	constexpr float kWarningTime = 45.0f;//予告のエフェクトの長さ
	constexpr float kEffectScale = 20.0f;//エフェクトは1/20で作っている
	constexpr float kExplodeTime = 4.0f;//爆発の長さ//判定もこの長さだけ出す
	constexpr float kExplodeRadius = 190.0f;//爆発の判定の半径//予告の円に合わせる
	constexpr float kExplodePower = 30.0f;//爆発の攻撃力

	//仮の見た目
	constexpr float kDrawLength = 60.0f;//ミサイルの長さ
	constexpr float kDrawRadius = 12.0f;//ミサイルの太さ
}

EnemyBazookaMissile::EnemyBazookaMissile(std::weak_ptr<CharacterBase> owner, const Vector3& startPos, const Vector3& targetPos)
{
	m_owner = owner;
	m_startPos = startPos;
	m_targetPos = targetPos;
	m_pos = startPos;
	m_prevPos = startPos;

	// TODO(手順1) 発射位置から着弾位置までの距離を m_distance に入れる
	//   書き方: (m_targetPos - m_startPos).Magnitude()<-やっぱりこれがわからない
	m_distance = (m_targetPos - m_startPos).Magnitude();

	// TODO(手順6) sin で揺らすための「横の向き」「縦の向き」「幅」を決める
	//   考え方:
	//   - 揺れは「進む向きに直角な横」と「進む向きに直角な上」の2つの向きに出す。
	//     ワールドのX・Yで揺らすと、撃つ向きによって揺れ方が変わってしまう(真横に撃つと横揺れが前後揺れになる)。
	//   - 進む向き dir = (m_targetPos - m_startPos) / m_distance (距離が0なら (0,0,1) にしておく。0で割らないため)
	//   - 横 m_right = 真上(0,1,0) と dir の外積 → Normalize
	//       外積は「2つのベクトルの両方に直角な向き」を返すので、上と前の両方に直角 = 横になる。
	//       真上・真下に撃つと dir と真上が同じ向きで外積が 0 になるので、そのときは (1,0,0) にする。
	//   - 縦 m_up = dir と m_right の外積 → Normalize
	//       外積の向き(+か-か)は掛ける順番で逆になる。どちらになるか覚えなくていいように、
	//       m_up.y がマイナスなら -1 を掛けて「上が正」にそろえる。
	//   - 幅: m_sideWidth = min(m_distance * kSideWidthRate, kMaxSideWidth)
	//         m_upWidth   = min(m_distance * kUpWidthRate,   kMaxUpWidth)
	//       距離に比例させるのは、近くで撃ったときに大きく揺れすぎないようにするため。
	//       上限を付けるのは、遠くで撃ったときに画面の外まで膨らまないようにするため。
	//   - 1発ごとに変える: 幅に kMinWidthRate〜1.0 の乱数を掛ける、m_sideWidth の符号を GetRand(1) で決める(左右)。
	//       全部同じ軌道だと、何発も撃たれたときに単調に見えるため。
}

EnemyBazookaMissile::~EnemyBazookaMissile()
{
	// TODO(手順7) 予告のエフェクトが残っていたら止める
	//   書き方: if (m_warningEffect >= 0) StopEffekseer3DEffect(m_warningEffect);
	//   なぜ要るか: ミサイルが途中で消されたとき(敵を全部消すデバッグ・シーンの終わり)に、
	//   地面に赤い円だけが残らないようにするため。エフェクトは持ち主が消えても再生され続ける。
	//   判定(m_flyCol / m_attackCol)は shared_ptr なので、ここで何もしなくても一緒に消える
	//   (CollisionManager は weak_ptr で持っているので、消えたものは勝手に一覧から外れる)。
}

void EnemyBazookaMissile::Init()
{
	// TODO(手順7) 着弾点に予告の円を出す
	//   書き方: m_warningEffect = EffectManager::GetInstance().Play(AsyncData::KingmechMissileWarningEffect, m_targetPos, 0.0f, kEffectScale);
	//   - kEffectScale(20)は、Kingmech のミサイルのエフェクトが 1/20 の大きさで作ってあるため。
	//   - 読み込みは System.cpp の Bazooka の所に足してある(Kingmech がいないステージでも読むため)。
	//
	// TODO(手順7) 予告の円が外周に届くのと、ミサイルが着くのを同じにする
	//   考え方:
	//   - MissileWarning.efk は「45Fで橙の円が外周に届く」= 残り時間を表す作り。
	//   - ミサイルは割合で進むので、撃った瞬間に着くまでのフレーム数が分かる:
	//       着くまでのフレーム数 = m_distance / kSpeed   (100F なら 3000 ÷ 30)
	//       sin で曲がっても割合の進み方は同じなので、この時間は変わらない。
	//   - 再生の速さ = kWarningTime(45) / 着くまでのフレーム数
	//       100Fで着く → 0.45倍(ゆっくり)、30Fで着く → 1.5倍(速く)
	//   - 書き方: SetSpeedPlayingEffekseer3DEffect(m_warningEffect, 速さ);  (フレーム数が0のときは呼ばない)
	//   - EffectManager::SetOwnSpeed は使わない。あれはタイムスケールを打ち消すので、スローのときに予告だけ速くなる。
	//     SetSpeedPlayingEffekseer3DEffect なら、全体のタイムスケールに掛け算されるのでミサイルと一緒に遅くなる。
	//
	// TODO(手順9) 飛んでいる間の判定を作る
	//   書き方:
	//     m_flyCol = std::make_shared<EnemyBazookaMissileCol>();
	//     m_flyCol->ColInit({ .pos = m_pos, .offset = Vector3(0,0,0),
	//         .shape = std::make_unique<SphereShape>(kFlyColRadius),
	//         .tag = {Collider::Faction::Enemy, Collider::ColRole::None},
	//         .isActive = true, .isTrigger = true });
	//     m_flyCol->SetPos(m_pos);
	//   - ColRole は Attack にしない。Attack にすると触れた瞬間にダメージが入り、
	//     そのあと爆発の判定でもう1回ダメージが入ってしまう(2回当たる)。ダメージは爆発だけにする。
	//   - isTrigger = true: 押し戻しをしない(ミサイルがプレイヤーを押さないように)。
	//   - make_shared で作るのは、ColInit の中で shared_from_this() を使って CollisionManager に登録するため。
	//   - Init をコンストラクタと分けているのもこのため(コンストラクタの中ではまだ shared_ptr になっていない)。
	//   - 半径 kFlyColRadius(40)は 1Fで進む距離(30)より大きくしてある。小さいと、1Fでプレイヤーを飛び越えてすり抜ける。
}

void EnemyBazookaMissile::Update()
{
	// TODO(手順2) Phase で分ける
	//   考え方: Kingmech のミサイル(KingmechMissile)と同じ形。段階ごとにやることが違うので switch で分ける。
	//   - Phase::Fly     → UpdateFly()
	//   - Phase::Explode → m_timer += タイムスケール。kExplodeTime を超えたら Phase::Finished
	//                       (爆発の判定は寿命で消えるが、ミサイル自体は Finished になるまで EnemyManager が消さない)
	//   - Phase::Finished → 何もしない(EnemyManager が IsFinished() を見て消す)
	//   タイムスケールは System::GetInstance().GetTimeScale()。掛けておくと、スロー・ヒットストップで一緒に遅くなる。

	switch (m_phase)
	{
		case Phase::Fly:
			UpdateFly();
			break;
		case Phase::Explode:
			m_timer += System::GetInstance().GetTimeScale();
			if (m_timer >= kExplodeTime)
			{
				m_phase = Phase::Finished;
			}
			break;
		case Phase::Finished:
			break;	
	}

}

void EnemyBazookaMissile::Draw()
{
	// TODO(手順3) 仮の見た目を描く
	//   考え方:
	//   - エフェクトを作るまでの仮。まずは「どこを飛んでいるか」が見えれば十分。
	//   - 飛んでいる間(Phase::Fly)だけ描く。
	//   - 向き dir = m_pos - m_prevPos(今のフレームで動いた向き)。sin で曲がった方へ自然に頭が向く。
	//     止まっているとき(長さがほぼ0)は m_targetPos - m_startPos を使う。どちらも0なら描かない(Normalize で0で割るため)。
	//   - 書き方:
	//       Vector3 tail = m_pos - dir.Normalize() * kDrawLength;
	//       DrawCapsule3D(tail.ToDxLibVector(), m_pos.ToDxLibVector(), kDrawRadius, 8,
	//           GetColor(255, 120, 40), GetColor(255, 255, 255), TRUE);
	//   - 呼ぶのは EnemyManager::Draw(手順4)。
	if(m_phase == Phase::Fly)
	{
		Vector3 dir = m_pos - m_prevPos;
		//dirの長さがほぼ0なら、最初の向きを使う
		if(dir.Magnitude() < 0.001f)
		{
			dir = m_targetPos - m_startPos;
		}
		//尾を求める
		Vector3 tail = m_pos - dir.Normalize() * kDrawLength;
		DrawCapsule3D(tail.ToDxLibVector(), m_pos.ToDxLibVector(), kDrawRadius, 8,
			GetColor(255, 120, 40), GetColor(255, 255, 255), TRUE);
	}

}

void EnemyBazookaMissile::UpdateFly()
{
	// TODO(手順9) 前のフレームでプレイヤーか物に触れていたら、今の位置で爆発して return
	//   書き方: if (m_flyCol && m_flyCol->IsTouched()) { Explode(m_pos); return; }
	//   - 「前のフレーム」なのは、当たり判定(CollisionManager::Update)が EnemyManager::Update の後に動くため。
	//     このフレームで動いた結果の判定は、次のフレームの頭で分かる。

	// TODO(手順2) 割合 m_rate を進める
	//   考え方:
	//   - m_rate は「全体の何割まで進んだか」。0 = 発射位置、1 = 着弾位置、0.5 = 真ん中。
	//   - 1Fで進みたいのは「距離」(kSpeed = 30)なので、全体の距離で割って「割合」に直す:
	//       1Fで増やす割合 = kSpeed / m_distance        (3000 なら 0.01 = 1%)
	//   - 割らずに 0.01 のような固定の値を足すと、どの距離でも100Fで着く = 遠くに撃ったほど速く見えてしまう。
	//     距離で割れば、1Fで動く距離はいつも30で、遠いほど時間がかかる(ミサイルらしい動き)。
	//   - タイムスケールを掛ける: m_rate += kSpeed / m_distance * GetTimeScale()
	//   - m_distance が 0 のとき(プレイヤーが銃口の真ん前)は割れないので、m_rate = 1 にする。
	//   - 1 を超えたら 1 に止める。手順6の sin は rate が 1 ちょうどでずれが0になるので、超えると着弾点からずれる。
	//   - 進める前に m_prevPos = m_pos を覚えておく(手順3の向き、手順10のステージとの線に使う)。
	m_rate += kSpeed / m_distance * System::GetInstance().GetTimeScale();//1Fで進む割合
	if (m_rate > 1.0f) m_rate = 1.0f;
	if (m_distance == 0.0f) m_rate = 1.0f;
	m_prevPos = m_pos;

	// TODO(手順2) 位置を出す
	//   書き方: m_pos = CalcFlyPos(m_rate);
	//   最初は CalcFlyPos がまっすぐの位置だけ返すようにしておくと、手順6の前でも動きを確かめられる。
	m_pos = CalcFlyPos(m_rate);

	// TODO(手順10) ステージに当たったら、当たった所で爆発して return
	//   考え方:
	//   - ステージ(建物・床)は Collider ではなくモデルなので、手順9の判定では分からない。
	//   - 前の位置 → 今の位置 の線分でステージのモデルに当たるかを調べる:
	//       MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(ステージのモデル, -1, m_prevPos.ToDxLibVector(), m_pos.ToDxLibVector());
	//       if (hit.HitFlag) { Explode(Vector3::FromDxLibVector(hit.HitPosition)); return; }
	//   - 線分にするのは、点で調べると1Fで30進む間に薄い壁を通り抜けるため。
	//   - ステージのモデルは m_flyCol->GetStageHandle() で取る(-1 なら調べない)。
	//   - 動いていないフレーム(ヒットストップでタイムスケール0)は線の長さが0になるので調べない。
	//   - 着弾点が床の上なら、最後のフレームで床に当たってそこで爆発する(着弾点とほぼ同じ所なので問題ない)。

	// TODO(手順8) 着弾点に着いたら(m_rate >= 1)、着弾点で爆発して return
	//   書き方: if (m_rate >= 1.0f) { Explode(m_targetPos); return; }

	// TODO(手順9) 飛んでいる間の判定をミサイルの位置へ動かす
	//   書き方: if (m_flyCol) m_flyCol->SetPos(m_pos);
}

Vector3 EnemyBazookaMissile::CalcFlyPos(float rate) const
{
	// TODO(手順2) まっすぐの線の上の位置を返す
	//   書き方: Vector3 pos = Vector3::Lerp(m_startPos, m_targetPos, rate);
	//   Lerp は「始点と終点の間の rate のところ」の座標。rate 0 で始点、1 で終点。
	Vector3 pos = Vector3::Lerp(m_startPos, m_targetPos, rate);

	// TODO(手順6) 横と縦のずれを足す
	//   書き方:
	//     pos += m_right * (m_sideWidth * sinf(DX_PI_F * rate * kSideWaveNum));
	//     pos += m_up    * (m_upWidth   * sinf(DX_PI_F * rate * kUpWaveNum));
	//   考え方:
	//   - 「そこまでの距離を1として」= rate を sin の中に入れる。
	//   - sin の中身を π × rate × 整数 にすると、rate = 0 と rate = 1 で sin がちょうど 0 になる。
	//     → 必ず銃口から出て、必ず着弾点に落ちる(途中だけ膨らむ)。
	//   - 整数が 1 なら山1つ(弧)、2 ならS字。整数でない値にすると着弾点からずれるので注意。
	//   - 縦の山が1つで幅がプラスなら、途中は必ず上に膨らむ(地面にもぐらない)。
	//     縦を2以上にすると下にも膨らむので、手順10でステージに当たって途中で爆発することがある。
	//   - 横と縦で山の数を変えると、らせんのような動きにもなる。

	return pos;//仮//手順2で pos を返す
}

void EnemyBazookaMissile::Explode(const Vector3& pos)
{
	// TODO(手順8) 爆発に切り替える
	//   やること:
	//   - m_phase = Phase::Explode、m_timer = 0、m_pos = pos
	//   - 予告の円を止める: StopEffekseer3DEffect(m_warningEffect)、m_warningEffect = -1
	//       途中でプレイヤーや壁に当たって爆発したとき、地面に円だけ残らないように。
	//   - 飛んでいる間の判定を消す: m_flyCol.reset()
	//       もう要らないので消す。CollisionManager は weak_ptr で持っているので、消えたものは勝手に外れる。
	//   - 爆発のエフェクト: EffectManager::GetInstance().Play(AsyncData::KingmechMissileExplodeEffect, m_pos, 0.0f, kEffectScale);
	//   - 爆発の判定: CreateExplodeCol()
	//   爆発の理由(着弾点に着いた・触れた・ステージに当たった)が何でも、ここを通るようにしておく。
	//   そうすると、爆発のときにやることを1か所で直せる。
}

void EnemyBazookaMissile::CreateExplodeCol()
{
	// TODO(手順8) 爆発の攻撃判定を出す
	//   考え方:
	//   - KingmechMissile::CreateExplodeCol と同じ作り。見比べながら書くとよい。
	//   - FixedAttackCol を使う。普通の AttackCol は毎フレーム持ち主(敵)の位置へ動くので、
	//     爆発した場所に置いておけない。FixedAttackCol は ApplyPos だけ「決めた位置に置く」に変えたもの。
	//   - AttackData: attackPower = kExplodePower、knockBackPower = Vector3(20, 20, 0)、hitStopTime = 0.1、isKirimomi = true
	//       (攻撃CSVの値はミサイルには使っていない。ミサイルの強さはここの定数で決める)
	//   - m_attackCol = std::make_shared<FixedAttackCol>(m_owner, data, m_pos);
	//     m_attackCol->ColInit({ .pos = m_pos, .offset = Vector3(0,0,0),
	//         .shape = std::make_unique<SphereShape>(kExplodeRadius),
	//         .tag = {Collider::Faction::Enemy, Collider::ColRole::Attack},
	//         .isActive = true, .isTrigger = true, .lifeTime = kExplodeTime });
	//   - lifeTime を入れると、そのフレーム数で CollisionManager から外れる(自分で消さなくてよい)。
	//   - m_attackCol をメンバーで持つのは、CollisionManager が weak_ptr でしか持たないため。
	//     ローカル変数にすると関数を抜けた瞬間に消えて、判定が1回も出ない。
	//   - 持ち主(m_owner)は撃った敵。AttackCol は当たったときに持ち主を見るので、
	//     撃った敵が消え終わった後の爆発はダメージが入らない(今はそれでよしとしている)。
}
