#pragma once
#include "DxLib.h"
#include "../Math/Vector3.h"
#include <vector>

namespace
{
	//ポリゴンの当たり判定の配列の最大数
	constexpr int kMaxHitPolygon = 2048;
}

class Collider;
class CollisionChecker;

class FixNextPosition
{
public:
	FixNextPosition();
	~FixNextPosition();
	/// <summary>
	/// 次のフレームでの座標を修正
	/// </summary>
	void FixNextPos(Collider& colA, Collider& colB);
private:
	/// <summary>
	/// 球と球の衝突処理
	/// </summary>
	void FixNextPosSS(Collider& colA, Collider& colB);
	/// <summary>
	/// 球とポリゴンの衝突処理
	/// </summary>
	void FixNextPosSP(Collider& colA, Collider& colB);
	/// <summary>
	/// 球とBoxの衝突処理
	/// </summary>
	void FixNextPosSB(Collider& colA, Collider& colB);
	/// <summary>
	/// カプセルと球の衝突処理
	/// </summary>
	void FixNextPosCS(Collider& colA, Collider& colB);
	/// <summary>
	/// カプセルとカプセルの衝突処理
	/// </summary>
	void FixNextPosCC(Collider& colA, Collider& colB);
	/// <summary>
	/// カプセルとポリゴンの衝突処理
	/// </summary>
	void FixNextPosCP(Collider& colA, Collider& colB);


	/// <summary>
	/// 壁ポリゴンと床ポリゴンに分ける
	/// </summary>
	void AnalyzeWallAndFloor(MV1_COLL_RESULT_POLY_DIM hitDim,const Vector3& nextPos);
	/// <summary>
	/// 球とポリゴンの押し戻しベクトルを返す
	/// </summary>
	Vector3 OverlapVecSP(const Vector3& nextPos, std::vector<MV1_COLL_RESULT_POLY>& dim, float shortDistance);
	/// <summary>
	/// 壁と当たった時の処理 カプセル
	/// </summary>
	Vector3 HitWallCP(const Vector3& headPos, const Vector3& legPos, float shortDistance);
	/// <summary>
	/// 床の高さに合わせる処理 カプセル
	/// </summary>
	Vector3 HitFloorCP(Collider& other,const Vector3& legPos,const Vector3& headPos, float shortDistance);
	/// <summary>
	/// 天井に当たった時の処理 カプセル
	/// </summary>
	void HitRoofCP(Collider& other, const Vector3& headPos, float shortDistance);
private:
	std::vector<MV1_COLL_RESULT_POLY> m_wall = {};
	std::vector<MV1_COLL_RESULT_POLY> m_floorAndRoof = {};
};

