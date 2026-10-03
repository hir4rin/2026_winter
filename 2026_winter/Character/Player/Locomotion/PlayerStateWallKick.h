#pragma once
#include "PlayerState.h"
#include "../../../Math/Vector3.h"
class Input;

class PlayerStateWallKick : public PlayerState
{
public:
	PlayerStateWallKick(std::weak_ptr<Player> player);
	virtual ~PlayerStateWallKick();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;

private:
	float m_gravity = 0.0f;
	Vector3 m_InitVel = Vector3(0, 0, 0);
};
