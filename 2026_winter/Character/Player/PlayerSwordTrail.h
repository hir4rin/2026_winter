#pragma once
#include <vector>
#include "Math/Vector3.h"

//刀の軌跡(白い風切り)
//刀身の根元と先の位置を毎フレーム覚え、直近の数フレームをなめらかにつないだ帯を描く
//古い点から消えるので、刃先の側へ縮んで消える
class PlayerSwordTrail
{
public:
	PlayerSwordTrail();
	~PlayerSwordTrail();

	//このフレームに軌跡を伸ばすか//毎フレーム呼ぶ(呼ばなければ伸ばさない)
	void SetEmitting(bool isEmitting) { m_isEmitting = isEmitting; }
	//ゲームのフレームが進んだときに1回呼ぶ//timeScaleで点を古くする
	void Sample(const Vector3& base, const Vector3& tip, float timeScale);
	void Draw();

private:
	struct Point
	{
		Vector3 base;
		Vector3 tip;
		float age = 0.0f;//出てからのフレーム数
		int segment = 0;//続けて伸ばした区切り(区切りが違う点はつながない)
	};
	struct Vertex
	{
		Vector3 base;
		Vector3 tip;
		float age = 0.0f;
	};
	//点の並び(古い順)をなめらかにした頂点を作る
	static void BuildVertices(const std::vector<Point>& points, int begin, int end, std::vector<Vertex>& out);
	static Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);
	void DrawStrip(const std::vector<Vertex>& vertices, int graph, int blendMode, float alpha);

	std::vector<Point> m_points;
	std::vector<Vertex> m_vertices;//描くときの作業用
	bool m_isEmitting = false;
	bool m_wasEmitting = false;
	int m_segment = 0;
	int m_streakGraph = -1;//細い線の束
	int m_edgeGraph = -1;//先のふちの光
	int m_hazeGraph = -1;//線の間のもや
};
