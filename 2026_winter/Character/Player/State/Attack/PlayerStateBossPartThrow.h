#pragma once
#include "Character/Player/PlayerState.h"

class EnemyBase;

//切り落としたボスのパーツ(ガトリング)を空中で投げ返す//全部自動//終わったら落下
class PlayerStateBossPartThrow : public PlayerState
{
public:
	PlayerStateBossPartThrow(std::weak_ptr<Player> player);
	virtual ~PlayerStateBossPartThrow();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;
	bool IsInvincible()const override { return true; }//演出中は被弾しない

private:
	void ThrowPart();//ボスに投げさせる(1回だけ)

private:
	std::weak_ptr<EnemyBase> m_target;//パーツを切り落としたボス
	bool m_isThrown = false;
	float m_throwFrame = 0.0f;//投げるフレーム
};
