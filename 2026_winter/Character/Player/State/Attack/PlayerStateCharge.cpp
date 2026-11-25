#include "PlayerStateCharge.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Render/EffectManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	constexpr float kChargeThreshold = 15.0f;//溜め攻撃になるまでのフレーム//仮

	constexpr float kAnimSpeed = 0.8f;
	constexpr float kStartBlendFrame = 5.0f;

	//構えはじめ
	constexpr float kChargeStartEndAnimFrame = 9.0f;
	constexpr float kChargeStartAnimSpeed = 0.8f;

	


	constexpr float kDecelFrame = 8.0f;//最高速から止まるまでのフレーム数
	constexpr float kDecel = Game::kMoveSpeed / kDecelFrame;//1フレームに減る速さ

	constexpr float kChargeEffectScale = 20.0f;//溜めのエフェクト//1/20で作っている
}

PlayerStateCharge::PlayerStateCharge(std::weak_ptr<Player> player) :
	PlayerState(player)
{
}

PlayerStateCharge::~PlayerStateCharge()
{
}

void PlayerStateCharge::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	auto& input = Input::GetInstance();

	if (!input.IsPressed("Y"))
	{
		//この時点でYを押していないときは強攻撃に遷移
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::heavyAttack));
		return;
	}


	player->m_rb.m_vel = player->m_rb.m_moveVel;


	m_frame = 0.0f;
	m_isLoop = false;
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("ChargeStart"), false, kChargeStartAnimSpeed, kChargeStartEndAnimFrame, 0.0f, kStartBlendFrame);

	//溜めのエフェクト//+Zを前にして作っているので、ゲームでは180度回す
	const Vector3 forward = player->GetTargetVec();
	m_effectHandle = EffectManager::GetInstance().Play(AsyncData::ChargeChargeEffect, player->m_rb.GetPos(), atan2f(forward.x, forward.z) + DX_PI_F, kChargeEffectScale);
}

void PlayerStateCharge::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	UpdateMoveVel(Vector3(0, 0, 0), kDecel);

	//エフェクトを足元についていかせる
	if (m_effectHandle != -1)
	{
		const Vector3 pos = player->m_rb.GetPos();
		SetPosPlayingEffekseer3DEffect(m_effectHandle, pos.x, pos.y, pos.z);
	}

	//構え始めが終わったらループへ
	if (!m_isLoop && player->m_anim.GetAnimEndFlag())
	{
		m_isLoop = true;
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("ChargeLoop"), true, kAnimSpeed);
	}

	//TODO: 離したときの分岐(強攻撃 / 溜め攻撃)・回避

	m_frame += 1.0f * System::GetInstance().GetTimeScale();

	//指定フレーム以下の時は強攻撃に移動
	if (!input.IsPressed("Y") && m_frame < kChargeThreshold)
	{
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::heavyAttack));
		return;
	}

	//指定フレーム以上で貯め攻撃
	if (!input.IsPressed("Y") && m_frame >=  kChargeThreshold)
	{
		player->ChangeState(std::make_shared<PlayerStateChargeAttack>(m_owner));
		return;
	}


	player->m_anim.Update();
}

void PlayerStateCharge::Exit()
{
	if (m_effectHandle != -1)
	{
		StopEffekseer3DEffect(m_effectHandle);
		m_effectHandle = -1;
	}
}

void PlayerStateCharge::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Charge %.0f/%.0f", m_frame, kChargeThreshold);
}
