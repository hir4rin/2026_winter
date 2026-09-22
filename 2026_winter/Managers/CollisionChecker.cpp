#include "CollisionChecker.h"
#include "../Collider/Collider.h"
#include "../Collider/CapsuleShape.h"
#include "../Collider/PolygonShape.h"
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
	auto capsule = dynamic_cast<CapsuleShape*>(&colA.GetShape());

	//カプセルの始点と終点
	Vector3 posStart = colA.GetNextPos();
	Vector3 posEnd = posStart + capsule->GetEndPos();
	//球の中心座標
	Vector3 spherePos = colB.GetNextPos();

	//最短距離
	float shortDistance = colA.GetRadius() + colB.GetRadius();

	//カプセルの始点座標から球へのベクトル
	Vector3 StartToShpere = spherePos - posStart;
	//カプセルの始点から終点へのベクトル
	Vector3 StartToEnd = posEnd - posStart;

	//内積//射影ベクトルを求めることで、球の中心からの最近点を求める
	float dot = StartToShpere.Dot(StartToEnd);

	float t = dot / StartToEnd.sqMagnitude();

	t = std::clamp(t, 0.0f, 1.0f);

	//最短距離を出す
	Vector3 minPos = posStart + StartToEnd * t;

	float distance = (spherePos - minPos).Magnitude();

	//距離が最短距離より大きい場合は当たっていない
	if(distance >= shortDistance)
	{
		return false;
	}

	capsule->SetNearPos(minPos);


	return true;
}

bool CollisionChecker::CheckCollCC(Collider& colA, Collider& colB)
{
	// どちらかのカプセルの一つの座標から片方のカプセルに垂線を引いて
	//　お互いのカプセルに最も近い座標をそれぞれ出す

	auto capsuleA = dynamic_cast<CapsuleShape*>(&colA.GetShape());
	auto capsuleB = dynamic_cast<CapsuleShape*>(&colB.GetShape());

	//カプセルの始点と終点	
	Vector3 posStartA = colA.GetNextPos();
	Vector3 posEndA = posStartA + capsuleA->GetEndPos();

	Vector3 posStartB = colB.GetNextPos();
	Vector3 posEndB = posStartB + capsuleB->GetEndPos();

	//平行かどうか
	Vector3 capVecA = posEndA - posStartA;
	Vector3 capVecB = posEndB - posStartB;

	//外積から平行かどうかチェックする
	Vector3 cross = capVecA.Cross(capVecB);

	//平行な場合
	if (cross.sqMagnitude() <= 0.0f)
	{
		return CheckParallelCC(colA,colB);
	}

	//最短距離
	float shortDis = capsuleA->GetRadius() + capsuleB->GetRadius();
	//今の最短距離
	float currentShortDistance = 10000.0f;//始点同士を最短にする

	for (int i = 0; i < 2; i++)
	{
		//線分のそれぞれの座標
		Vector3 lineStart;
		Vector3 lineEnd;
		//最初にカプセルBに対してカプセルAのそれぞれの点からの最短座標を出す
		if (i == 0)
		{
			//線分CD
			lineStart = posStartB;
			lineEnd = posEndB;
		}
		//カプセルAに対してカプセルBのそれぞれの点からの最短座標
		else
		{
			//線分AB
			lineStart = posStartA;
			lineEnd = posEndA;
		}

		for (int j = 0; j < 2; j++)
		{
			//確認する座標
			Vector3 checkPos;
			//Aから線分CDにおろしたとき
			if (i == 0 && j == 0)checkPos = posStartA;
			//Bから
			if (i == 0 && j == 1)checkPos = posEndA;
			//Cから線分ABにおろしたとき
			if (i == 1 && j == 0)checkPos = posStartB;
			//Bから
			if (i == 1 && j == 1)checkPos = posEndB;

			//射影ベクトルを求めて最近点を求める
			//線分の始点から指定の座標へのベクトル
			Vector3 segStartToPoint = checkPos - lineStart;
			//線分のベクトル
			Vector3 segVec = lineEnd - lineStart;

			//内積
			float dot = segStartToPoint.Dot(segVec);
			//ABの長さの2上で割る
			float t = dot / segVec.sqMagnitude();
			//クランプ
			t = std::clamp(t,0.0f, 1.0f);
			//最短距離を出す
			Vector3 minPos = lineStart + (segVec * t);

			float shortDistance = (checkPos - minPos).Magnitude();

			//初回または前回の最短距離より小さいなら現在の最短距離とする
			if (shortDistance < currentShortDistance)
			{
				currentShortDistance = shortDistance;

				//最短座標を記録
				//線分CDに下した場合
				if (i == 0)
				{
					//衝突判定で使うので一番近い座標を覚えておく
					capsuleB->SetNearPos(minPos);
				}
				//線分ABに下した場合
				else
				{
					//衝突判定で使うので一番近い座標を覚えておく
					capsuleA->SetNearPos(minPos);
				}

			}

		}

	}

	//現在の最短距離が変形の合計より大きいなら当たっていない
	if (currentShortDistance >= shortDis)
	{
		return false;
	}


	return true;
}

bool CollisionChecker::CheckCollCCVerDxLib(Collider& colA, Collider& colB)
{

	auto capsuleA = dynamic_cast<CapsuleShape*>(&colA.GetShape());
	auto capsuleB = dynamic_cast<CapsuleShape*>(&colB.GetShape());

	//カプセルの始点と終点	
	Vector3 posStartA = colA.GetNextPos();
	Vector3 posEndA = posStartA + capsuleA->GetEndPos();

	Vector3 posStartB = colB.GetNextPos();
	Vector3 posEndB = posStartB + capsuleB->GetEndPos();

	return HitCheck_Capsule_Capsule(posStartA.ToDxLibVector(),
		posEndA.ToDxLibVector(),
		capsuleA->GetRadius(),
		posStartB.ToDxLibVector(),
		posEndB.ToDxLibVector(),
		capsuleB->GetRadius()
	);
}

bool CollisionChecker::CheckCollSP(Collider& colA, Collider& colB)
{
	auto polygonCol = dynamic_cast<PolygonShape*>(&colB.GetShape());

	//当たっているポリゴンの数//第2引数の-1は、すべてのポリゴンをチェックするため
	auto hitDim = MV1CollCheck_Sphere(
		polygonCol->GetModelHandle(),
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
	auto capsule = dynamic_cast<CapsuleShape*>(&colA.GetShape());
	auto polygon = dynamic_cast<PolygonShape*>(&colB.GetShape());

	//カプセルの始点と終点	
	Vector3 posStart = colA.GetNextPos();
	Vector3 posEnd = posStart + capsule->GetEndPos();

	//当たっているポリゴンの数//第2引数の-1は、すべてのポリゴンをチェックするため
	auto hitDim = MV1CollCheck_Capsule(
		polygon->GetModelHandle(),
		-1,
		posStart.ToDxLibVector(),
		posEnd.ToDxLibVector(),
		capsule->GetRadius(),
		-1
	);

	//当たっていない場合はfalseを返す
	if(hitDim.HitNum <= 0)
	{
		//検出したプレイヤーの周囲のポリゴン情報を解放する
		//MV1CollCheck_Capsuleで確保したメモリを動的に確保しているため、使用後は必ず解放する必要がある
		MV1CollResultPolyDimTerminate(hitDim);

		//CCDの衝突判定チェック(線分にして通り抜けていないかチェック)
		auto startLineHitDim = MV1CollCheck_Line(
			polygon->GetModelHandle(),
			-1,
			colA.m_rb.GetPos().ToDxLibVector(),
			posStart.ToDxLibVector()
		);
		//当たっていたら
		if (startLineHitDim.HitFlag)
		{
			//CCD判定をした
			polygon->SetIsCCD(true);

			//当たり判定に使うので、保存
			polygon->SetLineHit(startLineHitDim);
			return true;
		}
		else
		{
			Vector3 endPos = colA.m_rb.GetPos() + capsule->GetEndPos();
			Vector3 nextEndPos = colA.GetNextPos() + capsule->GetEndPos();

			auto endLineHitDim = MV1CollCheck_Line(
				polygon->GetModelHandle(),
				-1,
				endPos.ToDxLibVector(),
				nextEndPos.ToDxLibVector()
			);

			if (endLineHitDim.HitFlag)
			{
				//CCD判定をした
				polygon->SetIsCCD(true);

				//当たり判定に使うので保存
				polygon->SetLineHit(endLineHitDim);
				return true;
			}
			else
			{
				//当たっていない
				return false;
			}

		}
	}

	if (colA.m_isTrigger)
	{
		// 検出したプレイヤーの周囲のポリゴン情報を開放する
		MV1CollResultPolyDimTerminate(hitDim);
	}
	else
	{
		//当たり判定に使うので保存
		polygon->SetHitDim(hitDim);
	}


	return true;
}

bool CollisionChecker::CheckParallelCC(Collider& colA, Collider& colB)
{
	auto capsuleA = dynamic_cast<CapsuleShape*>(&colA.GetShape());
	auto capsuleB = dynamic_cast<CapsuleShape*>(&colB.GetShape());

	//カプセルの始点と終点	
	Vector3 cPosA = colA.GetNextPos();
	Vector3 cPosB = cPosA + capsuleA->GetEndPos();

	Vector3 cPosC = colB.GetNextPos();
	Vector3 cPosD = cPosC + capsuleB->GetEndPos();

	//最短距離
	float shortDis = capsuleA->GetRadius() + capsuleB->GetRadius();

	//各距離をチェック
	Vector3 ac = cPosC - cPosA;
	Vector3 ad = cPosD - cPosA;
	Vector3 bc = cPosC - cPosB;
	Vector3 bd = cPosD - cPosB;
	//最短距離を出す
	float dis = ac.Magnitude();
	//一度入れておく
	Vector3 nearPosA = cPosA;
	Vector3 nearPosB = cPosC;

	//他の線分と比較して最短のパターンを見つける
	if (dis > ad.Magnitude())	
	{
		dis = ad.Magnitude();
		nearPosA = cPosA;
		nearPosB = cPosD;
	}
	if (dis > bc.Magnitude())
	{
		dis = bc.Magnitude();
		nearPosA = cPosB;
		nearPosB = cPosC;
	}
	if (dis > bd.Magnitude())
	{
		dis = bd.Magnitude();
		nearPosA = cPosB;
		nearPosB = cPosD;
	}
	//それぞれに保存
	capsuleA->SetNearPos(nearPosA);
	capsuleB->SetNearPos(nearPosB);

	//最短距離より大きいなら当たっていない
	if (dis >= shortDis)
	{
		return false;
	}

	//ここまで来たら当たってる
	return true;
}
