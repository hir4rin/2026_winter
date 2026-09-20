#pragma once
#include "ColliderShape.h"
class SphereShape :public ColliderShape
{
public:
	//explicitの説明
	//引数が1つのコンストラクタは、explicit を付けないと「float から SphereShape への変換ルール」としても働きます。
	explicit SphereShape(float radius) : m_radius(radius) {}

	ColliderType GetType() const override { return ColliderType::Sphere; }
	float GetRadius() const override { return m_radius; }

	void DebugDraw(const Vector3& center, unsigned int color) const override
	{
		DrawSphere3D(center.ToDxLibVector(), m_radius, 16, color, color, true);
	}

private:
	float m_radius;


};

