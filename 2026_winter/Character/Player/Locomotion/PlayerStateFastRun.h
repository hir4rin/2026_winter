#pragma once
#include "PlayerState.h"

class Input;

class PlayerStateFastRun :   public PlayerState
{
public:
    PlayerStateFastRun(std::weak_ptr<Player> player);
    virtual ~PlayerStateFastRun();
    void Enter() override;
    void Update() override;
	void Exit() override;

    void DebugDraw() override;
private:
	void Move(Input& input);//移動処理
};

