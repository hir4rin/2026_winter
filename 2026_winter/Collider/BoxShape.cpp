#include "BoxShape.h"

void BoxShape::DebugDraw(const Vector3& center, unsigned int color) const
{
	//Boxの描画は、中心と幅から、8点の頂点を求めて、そこから線を引いて描画する
	//Vector3 center = center;
	Vector3 half = m_halfExtents;

	Vector3 corners[8] = {
		center + Vector3(-half.x, -half.y, -half.z),
		center + Vector3(half.x, -half.y, -half.z),
		center + Vector3(half.x,  half.y, -half.z),
		center + Vector3(-half.x,  half.y, -half.z),
		center + Vector3(-half.x, -half.y,  half.z),
		center + Vector3(half.x, -half.y,  half.z),
		center + Vector3(half.x,  half.y,  half.z),
		center + Vector3(-half.x,  half.y,  half.z)
	};
	const int edges[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};
	for (auto& edge : edges)
	{
		DrawLine3D(corners[edge[0]].ToDxLibVector(), corners[edge[1]].ToDxLibVector(), color);
	}
}
