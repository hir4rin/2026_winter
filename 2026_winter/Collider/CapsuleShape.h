#pragma once
#include "ColliderShape.h"
class CapsuleShape :public ColliderShape
{
public:
    CapsuleShape(Vector3 endPos, float radius) : m_endPos(endPos), m_radius(radius) {}

    ColliderType GetType() const override { return ColliderType::Capsule; }
    float GetRadius() const override { return m_radius; }
    Vector3 GetEndPos() const { return m_endPos; }

private:
	Vector3 m_startPos; // カプセルの始点
	Vector3 m_endPos;// カプセルの終点
    float m_radius;
    //最近点
    Vector3 hitNearestPos;
    //最近点からの距離
    float hitNearestDistance;

};

