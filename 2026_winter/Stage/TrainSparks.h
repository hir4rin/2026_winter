#pragma once
#include "Math/Vector3.h"
#include <vector>

/// <summary>
/// 列車ステージの車輪の火花(NINJA GAIDEN 4 の列車を参考に)
/// 各車両の前後の台車の左右から、ランダムな間隔で火花(data/Effect/Train/TrainSpark.efk・左は TrainSparkL.efk)を出す。プレイヤーの近くだけ
/// </summary>
class TrainSparks
{
public:
	TrainSparks() = default;
	~TrainSparks();

	/// <summary>エフェクトを読み、台車の位置を決める</summary>
	/// <param name="stagePos">ステージのモデルの位置</param>
	void Init(const Vector3& stagePos);
	/// <summary>時間を進めて、時間になった台車から火花を出す(毎フレーム)</summary>
	void Update(const Vector3& playerPos);

private:
	struct Point
	{
		Vector3 pos;
		bool isLeft;//左側(火花を -X へ飛ばす)
		float timer;//次に出すまで(フレーム)
	};
	float RandomInterval()const;

	int m_effectRight = -1;//+X へ飛ぶ
	int m_effectLeft = -1;//-X へ飛ぶ
	std::vector<Point> m_points;
	std::vector<int> m_playingHandles;//再生中(シーンの終わりに止める)
};
