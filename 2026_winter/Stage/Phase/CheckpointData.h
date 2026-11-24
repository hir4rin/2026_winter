#pragma once
#include "Math/Vector3.h"
#include <string>

//チェックポイント1つ分
//data/Stage/Checkpoints/Stage005.csvなどの1行に対応する
struct CheckpointData
{
	std::string name;
	Vector3 position;//床の真ん中(エフェクトの原点)
	float rotYDeg = 0.0f;//戻ったときに向く方向(0で+Z)
	int phase = 0;//ここから始まる戦闘のフェーズ
	float radius = 150.0f;//踏んだら起動する範囲の半径(水平)
};
