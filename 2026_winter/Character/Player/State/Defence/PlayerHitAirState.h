#pragma once
#include "Character/Player/PlayerState.h"
#include "Character/CharacterBase.h"

//空中で被弾したとき//小さく吹っ飛んで落ちる
class PlayerHitAirState :
    public PlayerState
{
public:
    PlayerHitAirState(std::weak_ptr<Player> player, const CharacterBase::HitInfo& info);
    virtual ~PlayerHitAirState();
    void Enter() override;
    void Update() override;
    void Exit() override;
	void DebugDraw()override;

private:
	enum class Phase
	{
		Fly,//吹っ飛び
		Down,//倒れている
		GetUp,//起き上がり
	};

	void UpdateFly();//吹っ飛びの移動//着地したらDownへ
	void ChangeToLocomotion();//Idle/Moveへ

private:
	CharacterBase::HitInfo m_info;//被ダメ情報
    float m_gravity = 0.0f;//このステート中の重力の累積
	Phase m_phase = Phase::Fly;

};
