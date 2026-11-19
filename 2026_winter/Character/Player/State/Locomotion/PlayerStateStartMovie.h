#pragma once
#include "Character/Player/PlayerState.h"
class PlayerStateStartMovie :
    public PlayerState
{
public:
    PlayerStateStartMovie(std::weak_ptr<Player> player);
    virtual ~PlayerStateStartMovie();

    void Enter() override;
    void Update() override;
    void Exit() override;

    void DebugDraw()override;
private:
};
