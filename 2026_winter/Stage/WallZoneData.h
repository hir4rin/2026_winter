#pragma once
#include "../Math/Vector3.h"
#include "../Collider/Collider.h"
#include <string>

/// <summary>
/// 壁キック/壁走りできるゾーン1つ分のデータ
/// DataManagerでCSVとの間で読み書きし、WallZone(コライダー)に当てはめる
/// </summary>
struct WallZoneData
{
	Collider::ColRole type = Collider::ColRole::WallKickZone;//WallKickZoneかWallRunZone
	std::string name;//編集画面の一覧に出す名前(メモ用)
	Vector3 position;//ワールド座標での中心位置
	Vector3 halfExtents = Vector3(100.0f, 100.0f, 100.0f);//Boxの半分の大きさ
	float rotYDeg = 0.0f;//Y軸回転(度)
	bool isActive = true;//falseならゲーム中は判定しない
};
