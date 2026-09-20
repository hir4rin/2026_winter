#pragma once
#include "../Math/Vector3.h"
#include "ColliderSummery.h"

class ColliderShape
{
public:
	ColliderShape() = default;
	virtual ~ColliderShape() = default;

	virtual ColliderType GetType() const = 0;
	virtual float GetRadius() const = 0;//SphereやCapsuleの半径を返す
	virtual void DebugDraw(const Vector3& center, unsigned int color) const = 0;//デバッグ描画


};

