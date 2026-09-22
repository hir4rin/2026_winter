#include "CollisionChecker.h"
#include "../Collider/Collider.h"
#include "../Stage/Stage.h"
#include <algorithm>

CollisionChecker::CollisionChecker()
{
}

CollisionChecker::~CollisionChecker()
{
}

bool CollisionChecker::IsCollide(Collider& colA, Collider& colB)
{
	//コライダーのタイプによって当たり判定の関数を分ける
	auto typeA = colA.GetType();
	auto typeB = colB.GetType();

	bool isHit = false;
	//typeAによって分ける
	//球と
	if (typeA == ColliderType::Sphere)
	{
		//球
		if (typeB == ColliderType::Sphere)
		{
			isHit = CheckCollSS(colA, colB);
		}
		//カプセル
		else if (typeB == ColliderType::Capsule)
		{
			isHit = CheckCollCS(colB, colA);
		}
		//ポリゴン
		else if (typeB == ColliderType::Polygon)
		{
			isHit = CheckCollSP(colA, colB);
		}
		else if(typeB == ColliderType::Box)
		{
			isHit = CheckColSB(colA,colB);
		}
	}
	//カプセルと
	else if (typeA == ColliderType::Capsule)
	{
		//球
		if (typeB == ColliderType::Sphere)
		{
			isHit = CheckCollCS(colA, colB);
		}
		//カプセル
		else if (typeB == ColliderType::Capsule)
		{
			isHit = CheckCollCC(colA, colB);
		}
		//ポリゴン
		else if (typeB == ColliderType::Polygon)
		{
			isHit = CheckCollCP(colA, colB);
		}
	}
	//ポリゴンと
	else if (typeA == ColliderType::Polygon)
	{
		//球
		if (typeB == ColliderType::Sphere)
		{
			isHit = CheckCollSP(colB, colA);
		}
		//カプセル
		else if (typeB == ColliderType::Capsule)
		{
			isHit = CheckCollCP(colB, colA);
		}
		//ポリゴン
		else if (typeB == ColliderType::Polygon)
		{
			isHit = false;//ポリゴン同士の当たり判定は未実装
		}
	}


    return isHit;
}

bool CollisionChecker::CheckCollSS(Collider& colA, Collider& colB)
{
	//次のフレームでの座標を取得
	Vector3 posA = colA.GetNextPos();
	Vector3 posB = colB.GetNextPos();

	//距離を計算
	Vector3 AtoB = posB - posA;

	//距離が半径の和より大きい場合は当たっていない
	if(AtoB.Magnitude() > colA.GetRadius() + colB.GetRadius())
	{
		return false;
	}

	return true;
}
bool CollisionChecker::CheckColSB(Collider& colA, Collider& colB)
{
	//球とBOXの当たり判定
	//球の中心座標
	Vector3 sphereCenter = colA.GetNextPos();
	//BOXの中心座標
	Vector3 boxCenter = colB.GetNextPos();
	//BOXの半分のサイズ
	Vector3 boxHalfExtents = colB.GetHalfExtents();
	//球の中心座標をBOXのローカル座標に変換
	Vector3 localSphereCenter = sphereCenter - boxCenter;
	//球の中心座標をBOXの境界内に制限
	Vector3 closestPoint;
	closestPoint.x = (std::max)(-boxHalfExtents.x, (std::min)(localSphereCenter.x, boxHalfExtents.x));
	closestPoint.y = (std::max)(-boxHalfExtents.y, (std::min)(localSphereCenter.y, boxHalfExtents.y));
	closestPoint.z = (std::max)(-boxHalfExtents.z, (std::min)(localSphereCenter.z, boxHalfExtents.z));
	//最も近い点と球の中心との距離を計算
	Vector3 distanceVector = localSphereCenter - closestPoint;
	float distanceSquared = distanceVector.sqMagnitude();
	//距離が半径の2乗より小さい場合は当たっている
	return distanceSquared < (colA.GetRadius() * colA.GetRadius());
}

bool CollisionChecker::CheckCollCS(Collider& colA, Collider& colB)
{
	//球からカプセルに垂線を引いて球とカプセルの最短距離を求める
	return false;
}

bool CollisionChecker::CheckCollCC(Collider& colA, Collider& colB)
{
	return false;
}

bool CollisionChecker::CheckCollCCVerDxLib(Collider& colA, Collider& colB)
{
	return false;
}

bool CollisionChecker::CheckCollSP(Collider& colA, Collider& colB)
{
	auto polygonCol = dynamic_cast<Stage*>(&colB);

	//当たっているポリゴンの数//第2引数の-1は、すべてのポリゴンをチェックするため
	auto hitDim = MV1CollCheck_Sphere(
		polygonCol->GetStageModelHandle(),
		-1,
		colA.GetNextPos().ToDxLibVector(),
		colA.GetRadius(),
		-1
	);
	if (hitDim.HitNum <= 0)
	{
		//検出したプレイヤーの周囲のポリゴン情報を解放する
		//MV1CollCheck_Sphereで確保したメモリを動的に確保しているため、使用後は必ず解放する必要がある
		MV1CollResultPolyDimTerminate(hitDim);
		return false;
	}

	if (colA.m_isTrigger)
	{
		//トリガーの場合は押し戻しを行わないので、ポリゴン情報を解放する
		MV1CollResultPolyDimTerminate(hitDim);
	}
	else
	{
		//当たり判定の押し戻し処理に使うので、保存&& ポリゴン情報をまだ開放しない
		polygonCol->SetHitDim(hitDim);
	}

	return true;
}

bool CollisionChecker::CheckCollCP(Collider& colA, Collider& colB)
{
	return false;
}

bool CollisionChecker::CheckParallelCC(Collider& colA, Collider& colB)
{
	return false;
}
