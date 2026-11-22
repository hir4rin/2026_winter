#pragma once
#include "Collision/Collider.h"

//ミサイルが飛んでいる間の判定//ダメージは持たず、触れたかだけ覚える
class EnemyBazookaMissileCol :
	public Collider
{
public:
	EnemyBazookaMissileCol();
	virtual ~EnemyBazookaMissileCol();

	void OnCollision(Collider& other) override;
	void ApplyPos() override;

	void SetPos(const Vector3& pos);//ミサイルの位置に合わせる
	bool IsTouched()const { return m_isTouched; }//プレイヤーか物に触れたか
	int GetStageHandle()const;//ステージのモデル//無ければ-1

private:
	Vector3 m_pos;//ミサイルの位置
	bool m_isTouched = false;//プレイヤーか物に触れたか
};
