#pragma once
#include "Character/Player/PlayerState.h"

class EnemyBase;

//敵をガードブレイクさせたときのチャンス状態//時間内にYで打ち上げ派生
class PlayerStateGuardBreakChance : public PlayerState
{
public:
	PlayerStateGuardBreakChance(std::weak_ptr<Player> player);
	virtual ~PlayerStateGuardBreakChance();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;

private:
	void ToLocomotion();//Idle/Moveに戻る

private:
	std::weak_ptr<EnemyBase> m_target;//ブレイクした敵
	float m_timer = 0.0f;//チャンスの経過時間
};
