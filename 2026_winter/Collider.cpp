#include "Collider.h"

Collider::Collider()
{
}

Collider::~Collider()
{
}

void Collider::ColUpdate()
{
	//自身のタイムスケールの計算
	if (m_ownTimeScale != 1.0f)
	{
		m_timeCounter -= 1.0f;
		if (m_timeCounter <= 0.0f)
		{
			m_ownTimeScale = 1.0f;
		}
	}
	//寿命計算
	//float timeScale = System::GetInstance().GetTimeScale();
	float timeScale = 1.0f;
	if (m_lifeTime > 0.0f)
	{
		m_lifeTime -= 1.0f * timeScale * m_ownTimeScale;

		if (m_lifeTime <= 0.0f)
		{
			m_isActive = false;
			m_isLifeTimeLimited = true;
		}
	}
}
