#include "BattleManager.h"
#include "Main/System.h"
#include "AttackTokenManager.h"
#include "Render/BloodDecalManager.h"

BattleManager::BattleManager()
{
}

BattleManager::~BattleManager()
{
}

void BattleManager::Init()
{
	m_attackTokenMgr = std::make_shared<AttackTokenManager>();
	m_bloodDecalMgr = std::make_shared<BloodDecalManager>();
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

	//攻撃権の更新
	m_attackTokenMgr->Update();
	//床の血の跡の更新
	m_bloodDecalMgr->Update();
}

