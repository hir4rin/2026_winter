#pragma once
#include "PlayerState.h"

class Input;

class PlayerStateMove :   public PlayerState
{
public:
    PlayerStateMove(std::weak_ptr<Player> player);
    virtual ~PlayerStateMove();
    void Enter() override;
    void Update() override;
	void Exit() override;

    void DebugDraw() override;
private:
	void Move(Input& input);//移動処理
	int m_frame = 0;//このステートに入ってからのフレーム数
};

