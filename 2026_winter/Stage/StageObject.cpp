#include "StageObject.h"
#include "../Collider/BoxShape.h"

StageObject::StageObject()
{
}

StageObject::~StageObject()
{
}

void StageObject::Init(const Vector3& pos, const Vector3& halfExtents)
{
	ColInit({
		.pos = pos,
		.offset = Vector3(),
		.shape = std::make_unique<BoxShape>(halfExtents),
		.tag = {Collider::Faction::StaticObject, Collider::ColRole::None},
		.isActive = true
		});
}

void StageObject::OnCollision(Collider& other)
{
	//何もしない(押し戻しはCollisionManager側の共通処理に任せる)
}

void StageObject::ApplyPos()
{
	//ステージ制作モードでSetTransformされる以外、座標は動かないため何もしない
}

void StageObject::Draw() const
{
	DebugDraw();
}

void StageObject::SetTransform(const Vector3& pos, const Vector3& halfExtents)
{
	m_rb.m_pos = pos;
	SetShape(std::make_unique<BoxShape>(halfExtents));
}
