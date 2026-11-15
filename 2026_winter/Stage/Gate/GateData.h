#pragma once
#include "Math/Vector3.h"
#include <string>

/// <summary>
/// 扉(ゲート)1枚分のデータ(data/Stage/Gates/Stage004.csv の1行)
/// closePhase <= 今のフェーズ < openPhase の間だけ閉まる(フェーズが-1(終わり)なら開く)
/// </summary>
struct GateData
{
	std::string name;
	std::string model;//data/Stage/Gates/ のモデル名(.mv1 は付けない)。空なら当たり判定だけ
	Vector3 pos;//モデルの原点の位置
	float rotYDeg = 0.0f;
	int closePhase = 0;//このフェーズになったら閉まる(0なら最初から閉まっている)
	int openPhase = 1;//このフェーズになったら開く
	int delayFrame = 0;//フェーズが変わってから動き出すまで
	int moveFrame = 30;//開く・閉まるのにかかるフレーム
	Vector3 openMove;//開いたときの移動(扉の向き基準)
	Vector3 openScale = Vector3(1.0f, 1.0f, 1.0f);//開いたときの大きさ
	Vector3 colOffset;//当たり判定の箱の中心(原点から、扉の向き基準)
	Vector3 colHalf;//当たり判定の箱の半分の大きさ(xが0なら当たり判定なし)
	Vector3 size = Vector3(1.0f, 1.0f, 1.0f);//閉じているときの大きさ(CSVの最後の3列。省略で1)
};
