#pragma once
#include <memory>
#include <vector>
#include "Stage/Phase/EnemySpawnData.h"
#include "Stage/Phase/PhaseFlowData.h"

class EnemyManager;

//フェーズ進行の管理
class PhaseManager
{
public:
	PhaseManager() = default;
	~PhaseManager() = default;


	//最初のフェーズを始める
	void Init(std::weak_ptr<EnemyManager> enemyManager, const std::vector<EnemySpawnData>& spawnData,
		const std::vector<PhaseTransition>& transitions);

	void Update();

	int GetCurrentPhase()const { return m_currentPhase; }//0:未開始 -1:終了
	int GetPhaseKillCount()const;//このフェーズの撃破数

	void DebugDraw()const;

private:
	//次のフェーズ
	void ChangePhase(int next, PhaseEvent event);

	//条件が満たされたかどうか
	bool IsConditionFulfilled(const PhaseTransition& trans)const;

private:
	std::weak_ptr<EnemyManager> m_enemyManager;

	std::vector<PhaseTransition> m_transitions;//遷移表
	int m_currentPhase = 0;
	int m_phaseFrame = 0;//経過フレーム
	int m_killCountAtPhaseStart = 0;//フェーズ開始時の撃破数
};
