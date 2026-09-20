#pragma once
#include "PlayerState.h"
#include "../../CharacterBase.h"

class PlayerStateDie :
    public PlayerState
{
public:
    PlayerStateDie(std::weak_ptr<Player> player, CharacterBase::AttackData& data);
    virtual ~PlayerStateDie();
    void Enter() override;
    void Update() override;
    void Exit() override;
	void DebugDraw()override;
};

