#include "PlayerStateChargeAttack.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/System.h"
#include "Render/EffectManager.h"

namespace
{
	constexpr float kAnimSpeed = 0.8f;

	//ChargeAttack1
	constexpr float kChargeAttack1EndAnimFrame = 28.3f;
	constexpr float kChargeAttack1AnimSpeed = 0.8f;
	constexpr float kChargeAttack1BlendFrame = 5.0f;


	//ChargeAttack2
	constexpr float kChargeAttack2EndAnimFrame = 32.0f;
	constexpr float kChargeAttack2AnimSpeed = 0.8f;
	constexpr float kChargeAttack2BlendFrame = 5.0f;

	constexpr float kSlashEffectScale = 20.0f;//斬撃のエフェクト//1/20で作っている

	//攻撃データ//後ほどデータ化する
	constexpr float kAttackDamage = 100.0f;//攻撃力
	constexpr float kBrokenRate = 40.0f;//部位破壊率(%)
	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間
	constexpr float kAttackColOffset = 100.0f;//攻撃判定を前に出す距離
	constexpr float kAttackColRadius = 100.0f;//攻撃判定の半径//周りの敵をまとめて巻き込む

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

	if(phase == Phase::First)
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, animName, false, kChargeAttack1AnimSpeed, kChargeAttack1EndAnimFrame,0.0f, kChargeAttack1BlendFrame);
	}
	else
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, animName, false, kChargeAttack2AnimSpeed, kChargeAttack2EndAnimFrame, 0.0f, kChargeAttack2BlendFrame);
	}

	//斬撃のエフェクト//+Zを前にして作っているので、ゲームでは180度回す
	const Vector3 forward = player->GetTargetVec();
	const AsyncData effect = (phase == Phase::First) ? AsyncData::ChargeSlash1Effect : AsyncData::ChargeSlash2Effect;
	EffectManager::GetInstance().Play(effect, player->m_rb.GetPos(), atan2f(forward.x, forward.z) + DX_PI_F, kSlashEffectScale);


	//TODO: InitVerticalMove・CreateAttackCol
}
