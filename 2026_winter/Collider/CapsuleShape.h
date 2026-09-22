#pragma once
#include "ColliderShape.h"
class CapsuleShape :public ColliderShape
{
public:
    CapsuleShape(Vector3 endPos, float radius) : m_endPos(endPos), m_radius(radius) {}

    ColliderType GetType() const override { return ColliderType::Capsule; }

    //自分の座標とm_endPosの2点で構成されたカプセル
    float GetRadius() const override { return m_radius; }
    Vector3 GetEndPos() const { return m_endPos; }

    //最近点
	void SetNearPos(const Vector3& pos) { m_hitNearPos = pos; }
	Vector3 GetNearPos() const { return m_hitNearPos; }

    //最短距離
	float GetShortDistance() const { return m_hitNearestDistance; }
    void SetShortDistance(float distance) { m_hitNearestDistance = distance; }
	void DebugDraw(const Vector3& startPos, unsigned int color) const override;

private:
	//Vector3 m_startPos; // カプセルの始点
	Vector3 m_endPos;// カプセルの終点//座標からのローカル座標
    float m_radius;
    //最近点
    Vector3 m_hitNearPos = Vector3();
    //最近点からの距離
    float m_hitNearestDistance = 0.0f;

};

