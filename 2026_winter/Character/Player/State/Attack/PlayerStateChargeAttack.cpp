#include "PlayerStateChargeAttack.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/System.h"

namespace
{
	constexpr float kAnimSpeed = 0.8f;
}

PlayerStateChargeAttack::PlayerStateChargeAttack(std::weak_ptr<Player> player) :
	PlayerStateAttackBase(player)
{
}

PlayerStateChargeAttack::~PlayerStateChargeAttack()
{
}

void PlayerStateChargeAttack::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->SetPreventFall(true);
	DecideAttackDirection();

	StartPhase(Phase::First);
}

void PlayerStateChargeAttack::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	//1段目が終わったら2段目へ
	if (m_phase == Phase::First && player->m_anim.GetAnimEndFlag())
	{
		StartPhase(Phase::Second);
	}

	//TODO: 2段目が終わったらIdle/Move
	if (m_phase == Phase::Second && player->m_anim.GetAnimEndFlag())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
		return;
	}

	player->m_anim.Update();
}

void PlayerStateChargeAttack::Exit()
{
	auto player = m_owner.lock();
	if (player)
	{
		player->SetPreventFall(false);
		player->m_comboInfo.currentComboIndex = ComboIndex::None;
		player->m_comboInfo.isHit = false;
	}
	ReleaseAttackCol();
}

void PlayerStateChargeAttack::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:ChargeAttack %s",
		m_phase == Phase::First ? "First" : "Second");
}

void PlayerStateChargeAttack::StartPhase(Phase phase)
{
	auto player = m_owner.lock();
	if (!player) return;

	ReleaseAttackCol();//前の段の判定
	m_phase = phase;

	const std::string& animName = (phase == Phase::First) ? player->GetAnimName("ChargeAttack1") : player->GetAnimName("ChargeAttack2");
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, animName, false, kAnimSpeed);

	//TODO: InitVerticalMove・CreateAttackCol
}
