#include "PlayerStateCharge.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Render/EffectManager.h"
#include "Camera/CameraManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	constexpr float kChargeThreshold = 40.0f;//溜め攻撃になるまでのフレーム//仮

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
	m_effectRotY = atan2f(forward.x, forward.z) + DX_PI_F;
	m_phase = Phase::Charging;
	m_chargingEffect = PlayEffect(AsyncData::ChargeChargingEffect);
}

void PlayerStateCharge::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	UpdateMoveVel(Vector3(0, 0, 0), kDecel);

	//エフェクトを足元についていかせる
	const Vector3 pos = player->m_rb.GetPos();
	for (int handle : { m_chargingEffect, m_chargedEffect })
	{
		if (handle == -1) continue;
		SetPosPlayingEffekseer3DEffect(handle, pos.x, pos.y, pos.z);
	}

	//構え始めが終わったらループへ
	if (!m_isLoop && player->m_anim.GetAnimEndFlag())
	{
		m_isLoop = true;
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("ChargeLoop"), true, kAnimSpeed);
	}

	//TODO: 離したときの分岐(強攻撃 / 溜め攻撃)・回避

	m_frame += 1.0f * System::GetInstance().GetTimeScale();

	//溜まったら次の段階へ
	if (m_phase == Phase::Charging && m_frame >= kChargeThreshold)
	{
		ChangePhase(Phase::Charged);
	}

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
	if (m_chargingEffect != -1)
	{
		StopEffekseer3DEffect(m_chargingEffect);
		m_chargingEffect = -1;
	}
	if (m_chargedEffect != -1)
	{
		StopEffekseer3DEffect(m_chargedEffect);
		m_chargedEffect = -1;
	}
}

void PlayerStateCharge::ChangePhase(Phase next)
{
	m_phase = next;

	if (next == Phase::Charged)
	{
		//溜め中は新しく出すのだけ止めて、出ている煙は自然に消す
		if (m_chargingEffect != -1)
		{
			GetEffekseer3DManager()->StopRoot(m_chargingEffect);
			m_chargingEffect = -1;
		}
		PlayEffect(AsyncData::ChargeDoneEffect);//1回だけなのでハンドルは持たない
		m_chargedEffect = PlayEffect(AsyncData::ChargeChargedEffect);
	}
}

int PlayerStateCharge::PlayEffect(AsyncData key)
{
	auto player = m_owner.lock();
	if (!player) return -1;
	return EffectManager::GetInstance().Play(key, player->m_rb.GetPos(), m_effectRotY, kChargeEffectScale);
}

void PlayerStateCharge::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Charge %.0f/%.0f", m_frame, kChargeThreshold);
}
