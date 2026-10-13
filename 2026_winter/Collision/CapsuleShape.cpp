#include "CapsuleShape.h"

void CapsuleShape::DebugDraw(const Vector3& startPos, unsigned int color) const
{
	DrawCapsule3D(startPos.ToDxLibVector(), (startPos+m_endPos).ToDxLibVector(), m_radius,16, color, color,false);
}
