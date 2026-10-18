#pragma once
#include "PlayerStateAttackBase.h"
#include "Character/Player/PlayerEnums.h"

//ブレイク時の打ち上げ//スキル0→切り上げ
class PlayerStateGuardBreakAttack : public PlayerStateAttackBase
{
public:
	PlayerStateGuardBreakAttack(std::weak_ptr<Player> player);
	virtual ~PlayerStateGuardBreakAttack();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;

private:
	enum class Phase
	{
		Skill0,//打ち上げ
		UpAttack,//切り上げ
	};

	void StartPhase(Phase phase);//段の開始
	void UpdateSkill0();
	void UpdateUpAttack();
	void UpdateLaunchCamera();//注視点を敵に
	void AttackInputCheck();//先行入力
	void ToLocomotion();//Idle/Moveへ

private:
	Phase m_phase = Phase::Skill0;
	AttackType m_nextAttackType = AttackType::None;//予約された攻撃
	float m_endTimer = 0.0f;//アニメ終了後の経過
};
