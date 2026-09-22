#pragma once
#include "../Math/Vector3.h"

/// <summary>
/// ステージ上に配置する1つのBOX(当たり判定のみ)のデータ
/// StageCsvIOでCSVとの間で読み書きする最小単位
/// </summary>
struct StageObjectData
{
	Vector3 position;		//ワールド座標での中心位置
	Vector3 halfExtents;	//Boxの半分の大きさ(幅・高さ・奥行きの半分)
};
