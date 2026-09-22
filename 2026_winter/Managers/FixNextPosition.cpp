#include "FixNextPosition.h"
#include "FixNextPosition.h"
#include "../Collider/Collider.h"
#include "../Stage/Stage.h"
#include <algorithm>

namespace
{
	//押し戻しの値に足して密着するのを防ぐ
	constexpr float kOverlapGap = 1.0f;
	constexpr float kCheckUnder = -800.0f;
	constexpr float kCheckTop = 800.0f;
	constexpr float kWallThreshold = 0.5f;//境界線
	constexpr float kPushBackSplitRatio = 0.5f;//押し戻しを両者で分け合う割合(0.5倍にして両方が半分ずつ退く)
	constexpr float kStepClearanceHeight = 1.0f;//段差で突っかかるのを防ぐための壁ポリゴン判定の高さ余裕
}



FixNextPosition::FixNextPosition()
{
}

FixNextPosition::~FixNextPosition()
{
}

void FixNextPosition::FixNextPos(Collider& colA, Collider& colB)
{
	//初期化
	m_wall.clear();
	m_floorAndRoof.clear();

	//コライダーのタイプによって処理を分ける
	auto typeA = colA.GetType();
	auto typeB = colB.GetType();
	//球と
	if(typeA == ColliderType::Sphere)
	{
		//球
		if(typeB == ColliderType::Sphere)
		{
			FixNextPosSS(colA, colB);
		}
		//カプセル
		else if(typeB == ColliderType::Capsule)
		{
			FixNextPosCS(colB, colA);
		}
		//ポリゴン
		else if(typeB == ColliderType::Polygon)
		{
			FixNextPosSP(colA, colB);
		}
		//Box
		else if (typeB == ColliderType::Box)
		{
			FixNextPosSB(colA, colB);
		}
	}
	//カプセルと
	else if (typeA == ColliderType::Capsule)
	{
		//球
		if (typeB == ColliderType::Sphere)
		{
			FixNextPosCS(colA, colB);
		}
		//カプセル
		else if (typeB == ColliderType::Capsule)
		{
			FixNextPosCC(colA, colB);
		}
		//ポリゴン
		else if (typeB == ColliderType::Polygon)
		{
			FixNextPosCP(colA, colB);
		}
	}
	//ポリゴンと
	else if (typeA == ColliderType::Polygon)
	{
		//球
		if (typeB == ColliderType::Sphere)
		{
			FixNextPosSP(colB, colA);
		}
		//カプセル
		else if (typeB == ColliderType::Capsule)
		{
			FixNextPosCP(colB, colA);
		}
	}
}

void FixNextPosition::FixNextPosSS(Collider& colA, Collider& colB)
{
	//otherから自分へのベクトル//velを足した値
	Vector3 centerA = colA.GetWorldCenter() + colA.m_rb.GetVel();//自分の当たり判定の中心の座標
	Vector3 centerB = colB.GetWorldCenter() + colB.m_rb.GetVel();//相手の当たり判定の中心の座標

	auto AToBVec = (centerB - centerA);
	AToBVec.y = 0.0f;//Y軸の成分を0にする//水平面でのベクトルにする
	float distance = AToBVec.Magnitude();

	//距離が0の時は押し戻しをしない
	if (distance == 0)return;

	//重なりの深さ　＝　（自分の半径＋相手の半径）－距離
	float overlap = colA.GetRadius() + colB.GetRadius() - distance;

	bool isABoss = colA.GetTag().faction == Collider::Faction::Boss;
	bool isBBoss = colB.GetTag().faction == Collider::Faction::Boss;

	//どちらかがボスなら、ボスは動かさず、相手側だけを重なった分すべて押し戻す
	if (isABoss || isBBoss)
	{
		if (!isABoss)
		{
			colA.m_rb.m_vel += AToBVec.Normalize() * overlap * -1.0f;
		}
		if (!isBBoss)
		{
			colB.m_rb.m_vel += AToBVec.Normalize() * overlap;
		}
		return;
	}

	if (overlap > 0)
	{
		colA.m_rb.m_vel += AToBVec.Normalize() * overlap * kPushBackSplitRatio * -1;// 重なった分だけ押し戻す（0.5倍にして両方が半分ずつ退く）
		colB.m_rb.m_vel +=  AToBVec.Normalize() * overlap * kPushBackSplitRatio;// 重なった分だけ押し戻す（0.5倍にして両方が半分ずつ退く）
		return;
	}
	//重なっていない場合は、押し戻さない
	return;
}

void FixNextPosition::FixNextPosSP(Collider& colA, Collider& colB)
{
	auto polygonCol = dynamic_cast<Stage*>(&colB);

	//当たったポリゴンの情報
	auto& hitDim = polygonCol->GetHitDim();

	//球の座標
	Vector3 posA = colA.GetNextPos();

	//床ポリゴンと壁ポリゴンに分ける
	AnalyzeWallAndFloor(hitDim, posA);
	
	//床か天井に当たったか
	bool isFloorAndRoofHit = !m_floorAndRoof.empty();
	//壁に当たったか
	bool isWall = !m_wall.empty();

	//床と当たったなら
	if (isFloorAndRoofHit)
	{
		//補正するベクトルを返す
		Vector3 overlapVec = OverlapVecSP(posA, m_floorAndRoof, colA.GetRadius());
		//押し戻し
		colA.m_rb.m_vel += overlapVec;
		//修正方向が上向きなら床
		if (overlapVec.y > 0)
		{
			//床に当たっているというセット処理
			colA.SetIsFloor(true);
		}
	}
	//壁と当たったなら
	if (isWall)
	{
		//壁に当たっている

		//補正するベクトルを返す
		Vector3 overlapVec = OverlapVecSP(posA, m_wall, colA.GetRadius());

		colA.m_rb.m_vel += overlapVec;
	}

	//検出したプレイヤーの周囲のポリゴン情報を解放
	MV1CollResultPolyDimTerminate(hitDim);
}

void FixNextPosition::FixNextPosSB(Collider& colA, Collider& colB)
{
	//球の次の座標
	Vector3 spherePos = colA.GetNextPos();
	//Boxの次の座標
	Vector3 boxPos = colB.GetNextPos();
	//Boxの半分のサイズ
	Vector3 boxHalfExtents = colB.GetHalfExtents();

	//球の座標をBoxのローカル座標に変換
	Vector3 localSpherePos = spherePos - boxPos;

	//Boxの範囲内に収まるように制限し、Box内で球の中心に一番近い点を求める
	Vector3 closestPoint;
	closestPoint.x = (std::max)(-boxHalfExtents.x, (std::min)(localSpherePos.x, boxHalfExtents.x));
	closestPoint.y = (std::max)(-boxHalfExtents.y, (std::min)(localSpherePos.y, boxHalfExtents.y));
	closestPoint.z = (std::max)(-boxHalfExtents.z, (std::min)(localSpherePos.z, boxHalfExtents.z));

	//最近接点から球の中心へ向かうベクトル
	Vector3 diff = localSpherePos - closestPoint;
	float distance = diff.Magnitude();

	//距離が0の場合(球の中心がBoxの内部にある場合)は押し戻す方向が求まらないため何もしない
	if (distance == 0)return;

	//重なりの深さ　＝　球の半径　－　距離
	float overlap = colA.GetRadius() - distance;
	if (overlap > 0)
	{
		//重なった分だけ押し戻す(密着を防ぐ隙間を追加)//Box側は動かさない
		colA.m_rb.m_vel += diff.Normalize() * (overlap + kOverlapGap);
	}
}

void FixNextPosition::FixNextPosCS(Collider& colA, Collider& colB)
{
}

void FixNextPosition::FixNextPosCC(Collider& colA, Collider& colB)
{
}

void FixNextPosition::FixNextPosCP(Collider& colA, Collider& colB)
{
}

void FixNextPosition::AnalyzeWallAndFloor(MV1_COLL_RESULT_POLY_DIM hitDim, const Vector3& nextPos)
{
	//検出されたポリゴンの数だけ繰り返す
	for (int i = 0; i < hitDim.HitNum; ++i)
	{
		//法線のY成分が大きければ床、小さければ壁
		if (abs(hitDim.Dim[i].Normal.y) < kWallThreshold)
		{
			//壁ポリゴンと判断された場合でも、プレイヤーのY座標+1.0fより高いポリゴンのみ当たり判定を行う
			//段差で突っかかるのを防ぐため//?あまりわからない
			if(hitDim.Dim[i].Position[0].y > nextPos.y + kStepClearanceHeight ||
			   hitDim.Dim[i].Position[1].y > nextPos.y + kStepClearanceHeight ||
			   hitDim.Dim[i].Position[2].y > nextPos.y + kStepClearanceHeight)
			{
				//ポリゴンの数が列挙できる限界数に達していなかったらポリゴンを配列に保存する
				if (m_wall.size() < kMaxHitPolygon)
				{
					//ポリゴンの構造体のアドレスを壁ポリゴン配列に保存する
					m_wall.emplace_back(hitDim.Dim[i]);
				}
			}
			
		}
		//床ポリゴンの場合
		else
		{
			//ポリゴンの数が列挙できる限界数に達していなかったらポリゴン配列に保存
			if (m_floorAndRoof.size() < kMaxHitPolygon)
			{
				m_floorAndRoof.emplace_back(hitDim.Dim[i]);
			}
		}
	}
	
}

Vector3 FixNextPosition::OverlapVecSP(const Vector3& nextPos, std::vector<MV1_COLL_RESULT_POLY>& dim, float shortDistance)
{
	//わからん
	//なんで最期クランプしているのかもわからん
	//いったん自分でやってみる
	//



	//垂線を下ろして近い点を探して祭壇距離を求める
	float hitShortDis = FLT_MAX;//最短距離//FLT_MAXはfloat型の最大値
	//法線
	Vector3 normal = {};
	for (auto& poly : dim)
	{
		//内積と法線ベクトルからあたっている座標を求める//射影ベクトルの計算
		Vector3 PolyToPos = nextPos - Vector3::FromDxLibVector(poly.Position[0]);
		float dot = PolyToPos.Dot(Vector3::FromDxLibVector(poly.Normal));

		//ポリゴンと当たったオブジェクトが法線方向にいるなら向きを反転//???
		//dotではposからのベクトルなので、hitPosを求めるために反転させる必要がある
		//なぜ、Yの成分なのかわからん
		//if ((PolyToPos.y > 0 && poly.Normal.y > 0) || (PolyToPos.y < 0 && poly.Normal.y < 0))
		//{
		//	//ベクトルと法線が同じ向きなら反転が必要
		//	dot *= -1;
		//}
		//当たった座標
		Vector3 hitPos = Vector3::FromDxLibVector(poly.Normal) * dot + nextPos;
		//距離
		float dis = (hitPos - nextPos).Magnitude();

		//球の半径より遠い場合は無視
		if(dis > shortDistance)
		{
			continue;
		}
		//初回または前回より距離が短いなら
		if (hitShortDis > dis)
		{
			hitShortDis = dis;
			normal = Vector3::FromDxLibVector(poly.Normal);
		}
	}
	//押し戻し
	//どれくらい押し戻すのか
	float overlap = shortDistance - hitShortDis;
	overlap = std::clamp(overlap, 0.0f, shortDistance);
	overlap += kOverlapGap;

	return normal * overlap;
}

Vector3 FixNextPosition::HitWallCP(const Vector3& headPos, const Vector3& legPos, float shortDistance)
{
	return Vector3();
}

Vector3 FixNextPosition::HitFloorCP(Collider& other, const Vector3& legPos, const Vector3& headPos, float shortDistance)
{
	return Vector3();
}

void FixNextPosition::HitRoofCP(Collider& other, const Vector3& headPos, float shortDistance)
{
}
