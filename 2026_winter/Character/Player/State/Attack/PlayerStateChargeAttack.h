#pragma once
#include "PlayerStateAttackBase.h"

//溜め攻撃//2段で1つの技
class PlayerStateChargeAttack : public PlayerStateAttackBase
{
public:
	PlayerStateChargeAttack(std::weak_ptr<Player> player);
	virtual ~PlayerStateChargeAttack();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;

private:
	enum class Phase
	{
		First,//1段目
		Second,//2段目
	};

	void StartPhase(Phase phase);//段の開始
	void CreateAttackCol(const ComboNode& node) override;//球の大きさだけ変える
	void AttackMoveMent() override;

private:
	Phase m_phase = Phase::First;
	bool m_isSwingSePlayed = false;
};
