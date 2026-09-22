#include "FixNextPosition.h"
#include "FixNextPosition.h"
#include "../Collider/Collider.h"
#include "../Collider/PolygonShape.h"
#include "../Collider/CapsuleShape.h"
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
	auto polygonCol = dynamic_cast<PolygonShape*>(&colB.GetShape());

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
	auto capsuleA = dynamic_cast<CapsuleShape*>(&colA.GetShape());

	//AからBへのベクトル
	Vector3 AToB = colB.GetNextPos() - capsuleA->GetNearPos();

	//最短距離
	float shortDis = capsuleA->GetRadius() + colB.GetRadius();

	//どのくらい重なっているか
	float overlap = shortDis - AToB.Magnitude();
	overlap = std::clamp(overlap, 0.0f, shortDis);
	overlap += kOverlapGap;//余分に足しておく

	colA.m_rb.m_vel += AToB.Normalize() * -overlap * 0.5f;
	colB.m_rb.m_vel += AToB.Normalize() * overlap * 0.5f;

}

void FixNextPosition::FixNextPosCC(Collider& colA, Collider& colB)
{

	auto capsuleA = dynamic_cast<CapsuleShape*>(&colA.GetShape());
	auto capsuleB = dynamic_cast<CapsuleShape*>(&colB.GetShape());

	//AからBへのベクトル
	Vector3 AToB = capsuleB->GetNearPos() - capsuleA->GetNearPos();
	//最短距離
	float shortDis = capsuleA->GetRadius() + capsuleB->GetRadius();
	//どのくらい重ねっているか
	float overlap = shortDis - AToB.Magnitude();
	overlap = std::clamp(overlap, 0.0f, shortDis);
	overlap += kOverlapGap;

	//横方向にだけ動かしたいので
	AToB.y = 0.0f;

	colA.m_rb.m_vel += AToB.Normalize() * -overlap * 0.5f;
	colB.m_rb.m_vel += AToB.Normalize() * overlap * 0.5f;

}

void FixNextPosition::FixNextPosCP(Collider& colA, Collider& colB)
{
	auto capsule = dynamic_cast<CapsuleShape*>(&colA.GetShape());
	auto polygon = dynamic_cast<PolygonShape*>(&colB.GetShape());

	//CCDで判定しているなら
	if (polygon->IsCCD())
	{
		//当たったポリゴンの情報
		auto lineHit = polygon->GetLineHit();

		//壁なのか床なのかを見る
		//法線のY成分が大きければ床、小さければ壁
		//壁のとき
		if (abs(lineHit.Normal.y) < kWallThreshold)
		{
			//高さを保存
			float nextY = colA.GetNextPos().y;

			//次の位置
			Vector3 nextPos = lineHit.HitPosition;
			nextPos += Vector3(lineHit.Normal) * (capsule->GetRadius() + kOverlapGap);

			//始点から終点へのベクトル
			Vector3 StaartToEnd = (colA.GetNextPos() + capsule->GetEndPos()) - colA.GetNextPos();


			//座標確定
			colA.m_rb.m_pos = nextPos;
			colA.m_rb.m_pos.y = nextY;
			
			//移動量をリセット
			colA.m_rb.m_vel = Vector3();
		}
		//床の時
		else
		{
			if (lineHit.Normal.y > 0.0f)
			{
				//次の位置
				Vector3 nextPos = lineHit.HitPosition;
				nextPos.x = 0.0f;
				nextPos.z = 0.0f;
				nextPos.y += capsule->GetRadius() + kOverlapGap;

				//始点から終点へのベクトル
				Vector3 StartToEnd = (colA.GetNextPos() + capsule->GetEndPos()) - colA.GetNextPos();

				//座標確定
				colA.m_rb.m_pos.y = nextPos.y;
				//移動量をリセット
				colA.m_rb.m_vel.y = 0.0f;
			}
		}
		//CCDリセット
		polygon->SetIsCCD(false);
	}
	//CCDではない、ふつうに当たったとき
	else
	{
		//当たったポリゴンの情報
		auto& hitDim = polygon->GetHitDim();

		//カプセルの始点と終点の座標//startPosがY座標が上になるカプセル
		Vector3 endPos = colA.GetNextPos();//終点が座標
		Vector3 startPos = endPos + capsule->GetEndPos();//始点がオフセット

		//始点のほうが、終点より低い位置にあるなら入れ替える
		if (startPos.y < endPos.y)
		{
			Vector3 savePos = endPos;
			endPos = startPos;
			startPos = savePos;
		}
		float radius = capsule->GetRadius();

		//床ポリゴンと壁ポリゴンに分ける
		AnalyzeWallAndFloor(hitDim, endPos);

		//床か天井に当たったか
		bool isFloorAndRoof = !m_floorAndRoof.empty();
		//壁に当たったか
		bool isWall = !m_wall.empty();

		//床と当たったら
		if (isFloorAndRoof)
		{
			//ジャンプしているなら
			if (colA.m_rb.m_vel.y > 0.0f)
			{
				HitRoofCP(colA, startPos, radius);
			}
			else
			{
				//床の高さに合わせる
				HitFloorCP(colA, endPos, startPos, radius);
			}
		}
		//壁と当たっているなら
		if (isWall)
		{
			//壁に当たっているので、trueにする
			colA.SetIsWall(true);
			//補正ベクトルを返す
			Vector3 overlapVec = HitWallCP(startPos, endPos, radius);

			//ベクトルを補正
			colA.m_rb.m_vel += overlapVec;
		}
		// 検出したプレイヤーの周囲のポリゴン情報を開放する
		DxLib::MV1CollResultPolyDimTerminate(hitDim);
	}


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
	//垂線を下ろして近い点を探して最短距離を求める
	float hitShortDis = shortDistance;

	Vector3 top = headPos;
	top.y += shortDistance;
	Vector3 bot = legPos;
	bot.y -= shortDistance;

	//法線
	Vector3 norm = Vector3();
	for (auto& wall : m_wall)
	{
		//壁かチェック
		if (abs(wall.Normal.y) >= kWallThreshold)continue;
		VECTOR pos1 = wall.Position[0];
		VECTOR pos2 = wall.Position[1];
		VECTOR pos3 = wall.Position[2];

		//最短距離の2乗を返す
		float dis = Segment_Triangle_MinLength_Square(top.ToDxLibVector(), bot.ToDxLibVector(), pos1, pos2, pos3);
		//平方根を返す
		dis = sqrtf(dis);

		//初回または前回より距離が短いなら
		if (hitShortDis > dis)
		{
			//現状の最短
			hitShortDis = dis;
			//法線
			norm = wall.Normal;
		}
	}
	//押し戻し
	//どれくらい押し戻すか
	float overlap = shortDistance - hitShortDis;
	overlap = std::clamp(overlap, 0.0f, shortDistance);
	overlap += kOverlapGap;
	

	return norm.Normalize() * overlap;
}

bool FixNextPosition::HitFloorCP(Collider& other, const Vector3& legPos, const Vector3& headPos, float shortDistance)
{
	//足の真下にある床の中で最も高いY
	float finalFloorY = -FLT_MAX;
	bool hitFloor = false;

	//足の真下レイ(下方向)
	Vector3 rayStart = legPos;
	Vector3 rayEnd = rayStart + Vector3(0.0f, kCheckUnder, 0.0f);

	for (auto& floor : m_floorAndRoof)
	{
		//下向き法線なら床ではない
		if (floor.Normal.y < 0.0f)continue;

		VECTOR pos1 = floor.Position[0];
		VECTOR pos2 = floor.Position[1];
		VECTOR pos3 = floor.Position[2];

		//足の真下にあるポリゴンと交差チェック
		HITRESULT_LINE res = HitCheck_Line_Triangle(rayStart.ToDxLibVector(), rayEnd.ToDxLibVector(), pos1, pos2, pos3);

		if (res.HitFlag)
		{
			float hitPosY = res.Position.y;

			//より高い床を優先する//坂道で安定させるため
			if (hitPosY > finalFloorY)
			{
				finalFloorY = hitPosY;
				hitFloor = true;
			}
		}
	}

	if (hitFloor)
	{
		float newY = finalFloorY + shortDistance + kOverlapGap;

		other.m_rb.m_pos.y = newY;
		other.m_rb.m_vel.y = 0.0f;
		other.SetIsFloor(true);

		return true;
	}

	//足の真下に床が無かった場合キャラが坂の腹に刺さっている可能性あり
	//headからleg のラインが床を貫通していないかチェック
	float betweenY = -FLT_MAX;
	bool hitBetween = false;

	VECTOR head = headPos.ToDxLibVector();

	for (auto& floor : m_floorAndRoof)
	{
		if (floor.Normal.y < 0.0f) continue;

		VECTOR pos1 = floor.Position[0];
		VECTOR pos2 = floor.Position[1];
		VECTOR pos3 = floor.Position[2];

		HITRESULT_LINE res = HitCheck_Line_Triangle(head, rayStart.ToDxLibVector(), pos1, pos2, pos3);

		if (res.HitFlag)
		{
			if (res.Position.y > betweenY)
			{
				//間の床のY座標を保存
				betweenY = res.Position.y;
				hitBetween = true;
			}
		}
	}

	//headとlegの間でゆかが見つかった
	if (hitBetween)
	{
		float newY = betweenY + shortDistance + kOverlapGap;

		other.m_rb.m_pos.y = newY;
		other.m_rb.m_vel.y = 0.0f;
		other.SetIsFloor(true);
		return true;
	}

	return false;
}

void FixNextPosition::HitRoofCP(Collider& other, const Vector3& headPos, float shortDistance)
{

	//垂線を下して近い点を探して最短距離を求める
	float hitShortDis = shortDistance;
	
	//天井と当たったか
	bool isHitRoof = false;
	for (auto& roof : m_floorAndRoof)
	{
		//上向きの法線ベクトルなら飛ばす
		if (roof.Normal.y > 0.0f)continue;
		//頭の上にポリゴンがあるかチェック//線分とポリゴンの当たり判定
		HITRESULT_LINE lineResult = HitCheck_Line_Triangle(headPos.ToDxLibVector(), VAdd(headPos.ToDxLibVector(), VGet(0.0f, kCheckTop, 0.0f)),
			roof.Position[0], roof.Position[1], roof.Position[2]);

		//上のポリゴンがあったら
		if (lineResult.HitFlag)
		{
			//距離
			float dis = VSize(VSub(lineResult.Position, headPos.ToDxLibVector()));
			//初回または前回より距離が短いなら
			if (dis <  hitShortDis)
			{
				isHitRoof = true;
				//最短を更新
				hitShortDis = dis;
			}
		}
	}
	//当たっているなら
	if (isHitRoof)
	{
		//押し戻し
		//どれくらい押し戻すか
		float overlap = std::abs(shortDistance - hitShortDis);
		overlap = std::clamp(overlap, 0.0f, shortDistance);
		overlap += kOverlapGap;
		//法線
		Vector3 norm = Vector3(0.0f, -1.0f, 0.0f);
		other.m_rb.m_vel += norm * overlap;
	}

}
