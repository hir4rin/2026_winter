#pragma once
#include "PlayerState.h"
#include "../../../Math/Vector3.h"
class Input;

class PlayerStateFall : public PlayerState
{
	public:
	//canCoyoteJump:歩いて地面から離れたときだけtrue(落ち始めの数フレームはジャンプできる)
	PlayerStateFall(std::weak_ptr<Player> player, bool canCoyoteJump = false);
	virtual ~PlayerStateFall();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw()override;

private:
	void Move(Input& input);//落下中の移動処理
	Vector3 m_baseVel = {};//ジャンプ中の移動速度//ジャンプ中は空中での移動速度を一定にするために、ジャンプ開始時の移動速度を保存しておく
	float m_gravity = 0.0f;//このステート中の重力の累積//縦の速度は累積から毎フレーム作る
	bool m_canCoyoteJump = false;//コヨーテタイムのジャンプができるか
	int m_frame = 0;//このステートに入ってからのフレーム数
};

