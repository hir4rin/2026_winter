#include "BoxShape.h"

void BoxShape::DebugDraw(const Vector3& center, unsigned int color) const
{
	//Boxの描画は、中心と幅から、8点の頂点を求めて、そこから線を引いて描画する
	//Vector3 center = center;
	Vector3 half = m_halfExtents;

	//Y軸回転を反映するため、ローカルの頂点を回してから中心に足す
	Vector3 corners[8] = {
		center + LocalToWorldDir(Vector3(-half.x, -half.y, -half.z)),
		center + LocalToWorldDir(Vector3(half.x, -half.y, -half.z)),
		center + LocalToWorldDir(Vector3(half.x,  half.y, -half.z)),
		center + LocalToWorldDir(Vector3(-half.x,  half.y, -half.z)),
		center + LocalToWorldDir(Vector3(-half.x, -half.y,  half.z)),
		center + LocalToWorldDir(Vector3(half.x, -half.y,  half.z)),
		center + LocalToWorldDir(Vector3(half.x,  half.y,  half.z)),
		center + LocalToWorldDir(Vector3(-half.x,  half.y,  half.z))
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

Vector3 BoxShape::LocalToWorldDir(const Vector3& local) const
{
	//ローカルの+Zがワールドの(sin,0,cos)を向くように回す(カメラの角度と同じ向きの取り方)
	const float c = cosf(m_rotY);
	const float s = sinf(m_rotY);
	return Vector3(local.x * c + local.z * s, local.y, -local.x * s + local.z * c);
}

Vector3 BoxShape::WorldToLocalDir(const Vector3& world) const
{
	//LocalToWorldDirの逆回転
	const float c = cosf(m_rotY);
	const float s = sinf(m_rotY);
	return Vector3(world.x * c - world.z * s, world.y, world.x * s + world.z * c);
}
