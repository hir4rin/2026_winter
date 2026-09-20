#pragma once
#include "ColliderShape.h"
class BoxShape :
    public ColliderShape
{
public:
	explicit BoxShape(Vector3 halfExtents) : m_halfExtents(halfExtents) {}

	ColliderType GetType() const override { return ColliderType::Box; }

	float GetRadius() const override { return 0.0f; }
	Vector3 GetHalfExtents() const { return m_halfExtents; }

	void DebugDraw(const Vector3& center, unsigned int color) const override;

private:
	Vector3 m_halfExtents;
};

