#include "PlayerHitKirimomiState.h"
#include "Character/Player/Player.h"
#include "Main/Game.h"
#include "Main/Input.h"
#include "Main/System.h"

namespace
{
	constexpr float kDownEndFrame = 38.0f;//爆発のアニメを止めるフレーム//この後は立ち姿に戻る
	constexpr float kGetUpAnimEndFrame = 60.0f;//起き上がりアニメの終わり
}

PlayerHitKirimomiState::PlayerHitKirimomiState(std::weak_ptr<Player> player, const CharacterBase::HitInfo& info)
	:PlayerState(player),
	m_info(info)
{
}

PlayerHitKirimomiState::~PlayerHitKirimomiState()
{
}

void PlayerHitKirimomiState::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->m_anim.ChangeAnim(player->GetAnimName("HitKirimomi"), false, 0.5f, kDownEndFrame);

	m_gravity = 0.0f;
	m_phase = Phase::Fly;
}

void PlayerHitKirimomiState::Update()
{
	//空中でのhitようにまた別でステートを作る
	//吹き飛び中に入力があったら受け身を入れてみてもおもろい
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
			player->m_anim.ChangeAnim(player->GetAnimName("GetUp"), false,1.0f, kGetUpAnimEndFrame);
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

void PlayerHitKirimomiState::UpdateFly()
{
	//その向きのほうに吹っ飛ぶ
	auto player = m_owner.lock();
	if (!player) return;

	m_gravity += -Game::kGravity * System::GetInstance().GetTimeScale() * player->m_ownTimeScale;

	//落下中に床に着いたら着地//打ち上げ直後はまだ床の上にいるので判定しない
	bool isFalling = m_info.knockBackVel.y + m_gravity < 0.0f;
	if (player->IsFloor() && isFalling)
	{
		//死亡するならここで分岐を後でさせる
		m_phase = Phase::Down;
		return;//m_velは毎フレーム0なので止まる
	}

	//Enterは当たり判定の最中に呼ばれるので、ここで空中にする//IsLeftFloorでFallに移らないように
	if (!isFalling)player->SetIsFloor(false);

	player->m_rb.m_vel = m_info.knockBackVel + Vector3(0, m_gravity, 0);
}

void PlayerHitKirimomiState::ChangeToLocomotion()
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

void PlayerHitKirimomiState::Exit()
{
}

void PlayerHitKirimomiState::DebugDraw()
{
#ifdef _DEBUG
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:HitKirimomi");
#endif
}
