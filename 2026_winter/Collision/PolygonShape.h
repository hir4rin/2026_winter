#pragma once
#include "ColliderShape.h"
class PolygonShape :public ColliderShape
{
public:
	PolygonShape() = default;

	ColliderType GetType() const override { return ColliderType::Polygon; }
	float GetRadius() const override { return 0.0f; }
	void DebugDraw(const Vector3& center, unsigned int color) const override {}


	// 当たったポリゴンの情報
	void SetHitDim(MV1_COLL_RESULT_POLY_DIM& dim) { m_hitDim = dim; }
	MV1_COLL_RESULT_POLY_DIM& GetHitDim() { return m_hitDim; }
	//当たった線分の情報
	MV1_COLL_RESULT_POLY GetLineHit()const { return m_lineHit; }
	void SetLineHit(MV1_COLL_RESULT_POLY lineHit) { m_lineHit = lineHit; }

	//壁と床の近い座標
	Vector3 GetNearWallHitPos() const { return m_nearWallHitPos; }
	Vector3 GetNearFloorHitPos() const { return m_nearFloorHitPos; }
	//モデル
	int GetModelHandle() const { return m_modelHandle; }
	void SetModelHandle(int modelHandle) { m_modelHandle = modelHandle; }
	//CCDをしたか
	bool IsCCD()const { return m_isCCD; };
	void SetIsCCD(bool isCCD) { m_isCCD = isCCD; };
private:
	//当たり判定をするステージモデル
	int m_modelHandle = -1;
	//当たったポリゴンの情報
	MV1_COLL_RESULT_POLY_DIM m_hitDim;

	//線分とポリゴンの当たり判定情報
	MV1_COLL_RESULT_POLY m_lineHit;

	//当たった際の最も近い壁ポリゴンの座標
	Vector3 m_nearWallHitPos;

	//当たった際の最も近い床ポリゴンの座標
	Vector3 m_nearFloorHitPos;

	//連続的衝突判定（Continuous Collision Detection）をしたか
	//速度が早く、通り抜けてしまうようの対策
	bool m_isCCD;
};
