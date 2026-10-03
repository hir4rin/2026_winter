#include "WallZone.h"
#include "../Collider/BoxShape.h"
#include "../Collider/CapsuleShape.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr int kMaxCapsuleSampleNum = 32;//カプセルの線分を調べる点の最大数
}

WallZone::WallZone()
{
}

WallZone::~WallZone()
{
}

void WallZone::Init(const WallZoneData& data)
{
	m_data = data;
	ColInit({
		.pos = data.position,
		.offset = Vector3(),
		.shape = std::make_unique<BoxShape>(data.halfExtents, data.rotYDeg * DX_PI_F / 180.0f),
		.tag = {Collider::Faction::StaticObject, data.type},
		.isActive = data.isActive,
		.isTrigger = true,//押し戻しはしない
		});
}

void WallZone::SetData(const WallZoneData& data)
{
	m_data = data;
	m_rb.m_pos = data.position;
	SetShape(std::make_unique<BoxShape>(data.halfExtents, data.rotYDeg * DX_PI_F / 180.0f));
	SetTag({ Collider::Faction::StaticObject, data.type });
	SetIsActive(data.isActive);
}

bool WallZone::IsOverlap(const Collider& other) const
{
	if (!m_isActive)return false;

	const Vector3 start = other.GetWorldPos();
	if (other.GetType() == ColliderType::Capsule)
	{
		auto& capsule = static_cast<const CapsuleShape&>(other.GetShape());
		return IsOverlapCapsule(start, start + capsule.GetEndPos(), capsule.GetRadius());
	}
	if (other.GetType() == ColliderType::Sphere)
	{
		//球は長さ0のカプセルとして扱う
		return IsOverlapCapsule(start, start, other.GetRadius());
	}
	return false;
}

bool WallZone::IsOverlapCapsule(const Vector3& start, const Vector3& end, float radius) const
{
	auto& box = static_cast<const BoxShape&>(GetShape());
	const Vector3 center = GetWorldPos();
	const Vector3 half = box.GetHalfExtents();

	//線分上の点を何点か調べ、どれかが(半径ぶん膨らませた)Boxに入っていれば重なっている
	//点の間隔が半径以下になるように数を決める
	const float length = (end - start).Magnitude();
	int sampleNum = 1;
	if (radius > 0.0f)
	{
		sampleNum = static_cast<int>(std::ceil(length / radius)) + 1;
	}
	sampleNum = std::clamp(sampleNum, 1, kMaxCapsuleSampleNum);

	for (int i = 0; i < sampleNum; ++i)
	{
		const float t = (sampleNum == 1) ? 0.0f : static_cast<float>(i) / static_cast<float>(sampleNum - 1);
		const Vector3 point = start + (end - start) * t;

		//Boxのローカル座標にしてから、Box内の一番近い点を求める
		const Vector3 local = box.WorldToLocalDir(point - center);
		Vector3 closest;
		closest.x = std::clamp(local.x, -half.x, half.x);
		closest.y = std::clamp(local.y, -half.y, half.y);
		closest.z = std::clamp(local.z, -half.z, half.z);

		if ((local - closest).sqMagnitude() <= radius * radius)
		{
			return true;
		}
	}
	return false;
}

void WallZone::OnCollision(Collider& other)
{
	//何もしない(プレイヤー側からIsOverlapで問い合わせる)
}

void WallZone::ApplyPos()
{
	//編集画面でSetDataされる以外、座標は動かないため何もしない
}

void WallZone::DrawWithColor(unsigned int color) const
{
	GetShape().DebugDraw(GetWorldPos(), color);
}
