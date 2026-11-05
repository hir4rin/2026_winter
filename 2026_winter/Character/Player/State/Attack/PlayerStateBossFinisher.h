#pragma once
#include "Character/Player/PlayerState.h"
#include "Character/Animation.h"
#include "Math/Easing.h"
#include <string>

class EnemyBase;
struct ComboNode;

//ボスのトドメのムービー中のプレイヤー//動きはKingmechFinisherの指示に従う
class PlayerStateBossFinisher : public PlayerState
{
public:
	PlayerStateBossFinisher(std::weak_ptr<Player> player);
	virtual ~PlayerStateBossFinisher();
	void Enter() override;
	void Update() override;
	void Exit() override;
	void DebugDraw() override;
	bool IsInvincible()const override { return true; }

	void Warp(const Vector3& pos, const Vector3& front);//その場へ移す
	void MoveTo(const Vector3& goal, float frame, EasingMode easing, float power);
	void PlayAnim(const std::string& animName, bool isLoop, float speed, float endFrame, float blendFrame,
		RootMotionCancel cancel = RootMotionCancel::None);
	void Finish();

	float GetAnimFrame()const;
	const ComboNode& GetComboNode(int comboIndex)const;
	const std::string& GetAnimNameOf(const std::string& key)const;

private:
	std::weak_ptr<EnemyBase> m_target;

	bool m_isMoving = false;
	Vector3 m_moveStart;
	Vector3 m_moveGoal;
	float m_moveFrame = 0.0f;
	float m_moveTime = 0.0f;
	EasingMode m_moveEasing = EasingMode::EaseOut;
	float m_movePower = 1.0f;
};
