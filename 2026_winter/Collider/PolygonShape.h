#pragma once
#include "ColliderShape.h"
// Polygon shape is drawn via the model itself (MV1DrawModel), so DebugDraw is a no-op.
class PolygonShape :public ColliderShape
{
public:
	PolygonShape() = default;

	ColliderType GetType() const override { return ColliderType::Polygon; }
	float GetRadius() const override { return 0.0f; }
	void DebugDraw(const Vector3& center, unsigned int color) const override {}
};
