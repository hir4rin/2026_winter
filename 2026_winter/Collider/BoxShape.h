#pragma once
#include "ColliderShape.h"
class BoxShape :
    public ColliderShape
{
public:
	/// <param name="halfExtents">Boxの半分の大きさ</param>
	/// <param name="rotY">Y軸回転(ラジアン)。0ならAABBと同じ</param>
	explicit BoxShape(Vector3 halfExtents, float rotY = 0.0f) : m_halfExtents(halfExtents), m_rotY(rotY) {}

	ColliderType GetType() const override { return ColliderType::Box; }

	float GetRadius() const override { return 0.0f; }
	Vector3 GetHalfExtents() const { return m_halfExtents; }
	float GetRotY() const { return m_rotY; }

	void DebugDraw(const Vector3& center, unsigned int color) const override;

	/// <summary>Boxのローカル座標(中心基準・回転前)をワールドの向きに回す</summary>
	Vector3 LocalToWorldDir(const Vector3& local) const;
	/// <summary>ワールドの向きをBoxのローカルの向きに戻す(LocalToWorldDirの逆)</summary>
	Vector3 WorldToLocalDir(const Vector3& world) const;

private:
	Vector3 m_halfExtents;
	float m_rotY = 0.0f;//Y軸回転(ラジアン)//ローカルの+Zがワールドの(sin,0,cos)を向く
};

