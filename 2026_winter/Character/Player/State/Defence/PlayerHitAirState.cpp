#include "PlayerHitAirState.h"
#include "Character/Player/Player.h"
#include "Main/Game.h"
#include "Main/Input.h"
#include "Main/System.h"

namespace
{
	constexpr float kGravityRate = 1.0f;//重力の倍率//小さくするとゆっくり落ちる
	constexpr float kDownEndFrame = 38.0f;//爆発のアニメを止めるフレーム//この後は立ち姿に戻る
	constexpr float kGetUpAnimEndFrame = 60.0f;//起き上がりアニメの終わり
}

PlayerHitAirState::PlayerHitAirState(std::weak_ptr<Player> player, const CharacterBase::HitInfo& info)
	:PlayerState(player),
	m_info(info)
{
}

PlayerHitAirState::~PlayerHitAirState()
{
}

void PlayerHitAirState::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->m_anim.ChangeAnim(player->GetAnimName("HitKirimomi"), false, 0.5f, kDownEndFrame);

	//吹っ飛ぶ向きの逆(攻撃してきた側)を向く
	Vector3 faceDir = m_info.knockBackVel * -1.0f;
	faceDir.y = 0.0f;
	if (faceDir.Magnitude() > 0.0f) player->m_targetVec = faceDir.Normalize();

	m_gravity = 0.0f;
	m_phase = Phase::Fly;
}

void PlayerHitAirState::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	switch (m_phase)
	{
	case Phase::Fly:
		UpdateFly();
		break;
	case Phase::Down:
		//倒れきったら起き上がる
		if (player->m_anim.GetAnimEndFlag())
		{
			player->m_anim.ChangeAnim(player->GetAnimName("GetUp"), false, 1.0f, kGetUpAnimEndFrame);
			m_phase = Phase::GetUp;
		}
		break;
	case Phase::GetUp:
		if (player->m_anim.GetAnimEndFlag())
		{
			ChangeToLocomotion();
			return;
		}
		break;
	}

	player->m_anim.Update();
}

void PlayerHitAirState::UpdateFly()
{
	auto player = m_owner.lock();
	if (!player) return;

	m_gravity += -Game::kGravity * kGravityRate * System::GetInstance().GetTimeScale() * player->m_ownTimeScale;

	//落下中に床に着いたら着地
	bool isFalling = m_info.knockBackVel.y + m_gravity < 0.0f;
	if (player->IsFloor() && isFalling)
	{
		m_phase = Phase::Down;
		return;//m_velは毎フレーム0なので止まる
	}

	//Enterは当たり判定の最中に呼ばれるので、ここで空中にする
	if (!isFalling)player->SetIsFloor(false);

	player->m_rb.m_vel = m_info.knockBackVel + Vector3(0, m_gravity, 0);
}

void PlayerHitAirState::ChangeToLocomotion()
{
	auto player = m_owner.lock();
	if (!player) return;

	//入力の有無で状態を変える
	if (Input::GetInstance().IsLeftStickInput())
	{
		player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
		return;
	}
	player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
}

void PlayerHitAirState::Exit()
{
}

void PlayerHitAirState::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:HitAir");
#endif
}
