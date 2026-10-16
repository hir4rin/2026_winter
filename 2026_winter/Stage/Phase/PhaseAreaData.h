#pragma once
#include "Math/Vector3.h"
#include <string>

//フェーズ進行用のエリア(箱)1つ分
//data/Stage/PhaseAreas/Stage003.csvなどの1行に対応する
struct PhaseArea
{
	std::string name;//遷移表のparamに書く名前
	Vector3 position;//箱の中心(ワールド座標)
	Vector3 halfExtents = Vector3(100.0f, 100.0f, 100.0f);//箱の半分の大きさ
	float rotYDeg = 0.0f;//Y軸回転(度)
};
