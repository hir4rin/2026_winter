#pragma once
#include "Character/Player/PlayerState.h"

enum class AsyncData : int;

//Yを押している間の溜め//短く離したら強攻撃、溜めきったら溜め攻撃
class PlayerStateCharge : public PlayerState
{
public:
	enum class Phase
	{
		Charging,//ためている
		Charged,//溜まった
	};


public:
	PlayerStateCharge(std::weak_ptr<Player> player);
	virtual ~PlayerStateCharge();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;

private:
	void ChangePhase(Phase next);
	int PlayEffect(AsyncData key);//足元に溜めの向きで出す

	Phase m_phase = Phase::Charging;

	float m_frame = 0.0f;//溜めた時間
	bool m_isLoop = false;//ループのアニメに切り替えたか
	float m_effectRotY = 0.0f;//エフェクトの向き
	int m_chargingEffect = -1;//溜め中のエフェクト
	int m_chargedEffect = -1;//溜まった後のエフェクト
};
