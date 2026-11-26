#include "BattleManager.h"
#include "Main/System.h"
#include "AttackTokenManager.h"
#include "Render/BloodDecalManager.h"
#include "ResultRecorder.h"
#include "Render/EffectManager.h"

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
	m_resultRecorder = std::make_shared<ResultRecorder>();
}

void BattleManager::SetUltStart(int frames, const Vector3& burstPos)
{
	m_ultCount = frames;
	m_isUltimating = true;
	m_ultBurstPos = burstPos;
	EffectManager::GetInstance().StopHitWinds();//血殺中は風を消す
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
	//クリアタイム・コンボの切れ目
	m_resultRecorder->Update();
}

bool BattleManager::TakeScreenBloodRequest()
{
	const bool isRequested = m_isScreenBloodRequested;
	m_isScreenBloodRequested = false;
	return isRequested;
}

