#pragma once
#include "PlayerState.h"
#include "../../../Math/Vector3.h"
class Input;

class PlayerStateWallStay : public PlayerState
{
public:
	PlayerStateWallStay(std::weak_ptr<Player> player);
	virtual ~PlayerStateWallStay();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;

private:
};
