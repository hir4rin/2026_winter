#pragma once
#include "PlayerState.h"
#include "../../CharacterBase.h"

class PlayerStateHit :
    public PlayerState
{
public:
    PlayerStateHit(std::weak_ptr<Player> player, const CharacterBase::AttackData& data);
    virtual ~PlayerStateHit();
    void Enter() override;
    void Update() override;
    void Exit() override;
	void DebugDraw()override;
};

