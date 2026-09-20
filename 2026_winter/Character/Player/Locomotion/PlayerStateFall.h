#pragma once
#include "PlayerState.h"
#include "../../../Math/Vector3.h"
class Input;

class PlayerStateFall : public PlayerState
{
	public:
	PlayerStateFall(std::weak_ptr<Player> player);
	virtual ~PlayerStateFall();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;

private:
	void Move(Input& input);//落下中の移動処理
	Vector3 m_baseVel = {};//ジャンプ中の移動速度//ジャンプ中は空中での移動速度を一定にするために、ジャンプ開始時の移動速度を保存しておく
};

