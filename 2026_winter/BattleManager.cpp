#include "BattleManager.h"
#include "System.h"

BattleManager::BattleManager()
{
}

BattleManager::~BattleManager()
{
}

void BattleManager::Update()
{
	float timeScale = System::GetInstance().GetTimeScale();

	m_ultCount -= 1.0f * timeScale;

	if (m_ultCount <= 0)
	{
		m_ultCount = -1;
		m_isUltimating = false;
	}
}

