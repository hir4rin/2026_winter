#include "EnemyBazookaMissileCol.h"
#include "Stage/Stage.h"

//=============================================================================
// 手順9 ミサイルが飛んでいる間の判定
//
// なぜ専用のクラスを作るか:
// - Collider は OnCollision と ApplyPos が純粋仮想なので、そのままでは作れない。
// - AttackCol を使うと触れた瞬間にダメージが入る。ミサイルは「触れたら爆発」だけにして、
//   ダメージは爆発の判定(FixedAttackCol)だけにしたいので、ダメージを持たない判定が要る。
// - 触れたかどうかを覚えておいて、ミサイルが次のフレームの頭で IsTouched() を見る。
//=============================================================================

EnemyBazookaMissileCol::EnemyBazookaMissileCol()
{
}

EnemyBazookaMissileCol::~EnemyBazookaMissileCol()
{
}

void EnemyBazookaMissileCol::OnCollision(Collider& other)
{
	// TODO(手順9) 触れた相手を見て、爆発するものなら m_isTouched = true
	//   考え方:
	//   - CollisionManager は全部の判定の組み合わせを調べて、重なっていたら両方の OnCollision を呼ぶ。
	//     なので、ここには敵の体・ほかのミサイルなど何でも来る。相手のタグで選ぶ。
	//   - プレイヤー: other.GetFaction() == Faction::Player なら爆発。
	//       体・やられ判定・攻撃判定のどれでもよい(「触れたら爆発」と決めたため。斬っても爆発する)。
	//       ただし other.GetRole() == ColRole::JustDodge は除く。
	//       ジャスト回避の判定は体より大きく、触れただけで爆発すると回避が成功しにくくなるため。
	//       (爆発の判定は Attack なので、爆発に巻き込まれたときはちゃんとジャスト回避の判定に当たる)
	//   - 置いてある物: Faction::StaticObject なら爆発。
	//   - ほかの敵(Faction::Enemy / Boss)は無視する(同士討ちにしないと決めたため)。
}

void EnemyBazookaMissileCol::ApplyPos()
{
	// TODO(手順9) m_rb.m_pos = m_pos;
	//   ApplyPos は CollisionManager::Update の最後に呼ばれて、判定の位置を決める関数。
	//   AttackCol は持ち主の位置へ動かすが、これはミサイルが SetPos で入れた位置に置く。
}

void EnemyBazookaMissileCol::SetPos(const Vector3& pos)
{
	// TODO(手順9) 位置を入れる
	//   書き方:
	//     m_pos = pos;
	//     m_rb.m_pos = pos;
	//     m_rb.m_vel = Vector3(0, 0, 0);
	//   - m_rb.m_pos にも直接入れる理由: ApplyPos は判定を調べた「後」に呼ばれる。
	//     m_pos だけ入れると、このフレームの判定は前のフレームの位置で調べることになり、1F遅れる。
	//   - m_vel を0にするのは、CollisionManager が速度を足して位置を動かすため(ミサイルは自分で位置を決める)。
}

int EnemyBazookaMissileCol::GetStageHandle() const
{
	// TODO(手順10) ステージのモデルを返す
	//   書き方:
	//     auto stage = m_stage.lock();
	//     if (!stage) return -1;
	//     return stage->GetStageModelHandle();
	//   - m_stage は CollisionManager が毎フレーム全部の判定に入れてくれる(Collider のメンバー)。
	//     ミサイルはステージを持っていないので、判定から借りる。
	//   - 最初のフレーム(まだ CollisionManager が入れる前)は空なので -1 を返す。
	return -1;//仮
}
