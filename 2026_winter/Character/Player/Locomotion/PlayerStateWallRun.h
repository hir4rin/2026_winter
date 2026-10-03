#pragma once
#include "PlayerState.h"
#include "../../../Math/Vector3.h"
class Input;

class PlayerStateWallRun : public PlayerState
{
public:
	enum class WallRunDir
	{
		Left,
		Right
	};

public:
	PlayerStateWallRun(std::weak_ptr<Player> player);
	virtual ~PlayerStateWallRun();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;

private:
	WallRunDir m_wallRunDir = WallRunDir::Left;
	Vector3 m_wallRunDirVec = Vector3(0, 0, 0);
};
