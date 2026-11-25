#pragma once
#include "Character/Player/PlayerState.h"

//Yを押している間の溜め//短く離したら強攻撃、溜めきったら溜め攻撃
class PlayerStateCharge : public PlayerState
{
public:
	PlayerStateCharge(std::weak_ptr<Player> player);
	virtual ~PlayerStateCharge();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;

private:
	float m_frame = 0.0f;//溜めた時間
	bool m_isLoop = false;//ループのアニメに切り替えたか
};
