#pragma once
#include "PlayerState.h"
#include "PlayerEnums.h"

//落下攻撃の着地硬直
//着地モーション中は硬直し、指定フレームを過ぎたら回避・ジャンプなどでキャンセルできる
//弱攻撃(空中弱攻撃の最終段)と強攻撃(空中強攻撃)で硬直のフレームを変える
class PlayerStateAttackLanding :
	public PlayerState
{
public:
	PlayerStateAttackLanding(std::weak_ptr<Player> player, AttackType type);
	virtual ~PlayerStateAttackLanding();
	void Enter() override;
	void Update() override;
	void Exit() override;

	void DebugDraw()override;
private:
	//回避でキャンセルできるか
	bool CanDodgeCancel();
	//回避以外(攻撃・ジャンプ・確殺・移動)でキャンセルできるか
	bool CanActionCancel();
private:
	AttackType m_attackType;//どの攻撃からの着地か//lightAttackかheavyAttack
	float m_dodgeCancelFrame = 0.0f;//回避でキャンセルできるフレーム
	float m_actionCancelFrame = 0.0f;//回避以外でキャンセルできるフレーム//このフレームで硬直終了
	float m_landingTimer = 0.0f;//着地してからの経過フレーム
};
