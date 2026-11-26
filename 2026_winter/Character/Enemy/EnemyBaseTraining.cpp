#include "EnemyBase.h"
#include "Character/Enemy/State/EnemyStateBase.h"
#include "DxLib.h"
#include <vector>

//トレーニングの敵の動き(EnemyBase のメンバー。EnemyBase.cpp が長いので分けた)

namespace
{
	constexpr int kTrainingAttackInterval = 90;//攻撃してくる: Idleに戻ってから次の技までのフレーム
}

void EnemyBase::SetTraining(TrainingAi ai, bool isLockHp)
{
	m_trainingAi = ai;
	m_isTrainingLockHp = isLockHp;
	m_trainingAttackTimer = 0;
	//ふつう以外はIdleから動かない(EnemyManager が毎フレーム SetDebugIdle で反映する)
	m_isDebugStopAI = (ai != TrainingAi::Normal);
}

void EnemyBase::UpdateTraining()
{
	if (m_isLifeZero || m_isExecuted)return;
	if (m_isTrainingLockHp) m_hp = m_status.hp;

	const bool isIdle = std::dynamic_pointer_cast<EnemyIdle>(m_currentState) != nullptr;
	switch (m_trainingAi)
	{
	case TrainingAi::Guard:
		//ガードが終わってIdleに戻ったら、またガードする
		if (isIdle) ChangeState(std::make_shared<EnemyGuard>(GetWeakPtr()));
		break;
	case TrainingAi::Attack:
	{
		if (!isIdle)
		{
			m_trainingAttackTimer = 0;
			break;
		}
		if (++m_trainingAttackTimer < kTrainingAttackInterval)break;
		m_trainingAttackTimer = 0;
		//連撃の2撃目など(attackRange が0以下)は単体では出さない
		std::vector<int> candidates;
		for (int i = 0; i < static_cast<int>(m_attackList.size()); ++i)
		{
			if (m_attackList[i].attackRange > 0.0f) candidates.push_back(i);
		}
		if (candidates.empty())break;
		DebugExecuteAttack(candidates[GetRand(static_cast<int>(candidates.size()) - 1)]);
		break;
	}
	default:
		break;
	}
}
