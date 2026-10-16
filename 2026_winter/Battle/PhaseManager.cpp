#include "PhaseManager.h"
#include "Character/Enemy/EnemyManager.h"
#include "DxLib.h"
#include <cassert>

void PhaseManager::Init(std::weak_ptr<EnemyManager> enemyManager, const std::vector<EnemySpawnData>& spawnData,
	const std::vector<PhaseTransition>& transitions)
{
	m_enemyManager = enemyManager;
	m_currentPhase = 0;

	m_transitions = transitions;
	assert(!m_transitions.empty() && "遷移表CSVがありません");

	if (auto enemyMgr = m_enemyManager.lock())
	{
		enemyMgr->SetSpawnData(spawnData);
	}

	//Startの行から始める
	for (const auto& t : m_transitions)
	{
		if (t.from != 0 || t.condition != PhaseConditionType::Start)continue;
		ChangePhase(t.to, t.event);
		return;
	}
	assert(false && "遷移表にStartの行がありません");
}

void PhaseManager::Update()
{
	if (m_currentPhase <= 0)return;
	m_phaseFrame++;

	//次のフェーズへ進条件を満たしたかを調べ、満たしたら次のを出すようにする

	for (const auto& trans : m_transitions)
	{
		if (trans.from != m_currentPhase)continue;
		//条件が満たされているかチェック
		if (!IsConditionFulfilled(trans))continue;

		ChangePhase(trans.to, trans.event);
		return;
	}
}

void PhaseManager::ChangePhase(int next, PhaseEvent event)
{
	m_currentPhase = next;
	m_phaseFrame = 0;

	if (auto enemyMgr = m_enemyManager.lock())
	{
		m_killCountAtPhaseStart = enemyMgr->GetTotalKillCount();
		//-1(終了)なら敵は出さない
		if (next > 0) enemyMgr->StartPhase(next);
	}

	//eventの処理
	switch (event)
	{
	case PhaseEvent::BossAppear:
		//printfDx("BossAppear\n");//仮
		break;
	case PhaseEvent::GameClear:
		//printfDx("GameClear\n");//仮
		break;
	default:
		break;
	}

}

bool PhaseManager::IsConditionFulfilled(const PhaseTransition& trans) const
{
	auto enemyMgr = m_enemyManager.lock();
	if (!enemyMgr)return false;


	//一旦いまはこれだけ
	switch (trans.condition)
	{
	case PhaseConditionType::Start:
		return true;
	case PhaseConditionType::AllDead:
		if (trans.param.empty())return enemyMgr->IsPhaseEnemiesAllDead();
		return enemyMgr->IsGroupAllDead(trans.param);
		break;
	case PhaseConditionType::Timer:
		return m_phaseFrame >= trans.intParam;
	case PhaseConditionType::KillCount:
		return GetPhaseKillCount() >= trans.intParam;

	default:
		return false;
	}


	return false;
}

int PhaseManager::GetPhaseKillCount() const
{
	auto enemyMgr = m_enemyManager.lock();
	if (!enemyMgr)return 0;
	//開始時の撃破数を引く
	return enemyMgr->GetTotalKillCount() - m_killCountAtPhaseStart;
}

void PhaseManager::DebugDraw() const
{
	DrawFormatString(10, 150, GetColor(0, 255, 0), "Phase:%d Frame:%d Kill:%d",
		m_currentPhase, m_phaseFrame, GetPhaseKillCount());
}
