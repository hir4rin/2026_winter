#include "PlayerStateStartMovie.h"
#include "Character/Player/Player.h"


namespace
{
	constexpr float kJumpRoopStartFrame = 8.95f;
	constexpr float kJumpRoopEndFrame = 19.2f;
	constexpr float kAnimSpeed = 0.4f;
	constexpr float kBlendFrame = 5.0f;
}


PlayerStateStartMovie::PlayerStateStartMovie(std::weak_ptr<Player> player) :
	PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateStartMovie::~PlayerStateStartMovie()
{
}

void PlayerStateStartMovie::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JumpUp"), false, kAnimSpeed, kJumpRoopEndFrame,kJumpRoopStartFrame, kBlendFrame);
}

void PlayerStateStartMovie::Update()
{
	//自動で動かす
	auto player = m_owner.lock();
	if (!player) return;

	if (player->m_anim.GetAnimEndFlag())
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JumpUp"), false, kAnimSpeed, kJumpRoopEndFrame, kJumpRoopStartFrame, kBlendFrame);
		return;
	}

	player->m_anim.Update();
}

void PlayerStateStartMovie::Exit()
{
}

void PlayerStateStartMovie::DebugDraw()
{
}
