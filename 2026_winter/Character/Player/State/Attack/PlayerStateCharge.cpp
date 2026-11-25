#include "PlayerStateCharge.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"

namespace
{
	constexpr float kChargeThreshold = 15.0f;//溜め攻撃になるまでのフレーム//仮

	const std::string kStartAnim = "Power_Attack_Start";//構え始め
	const std::string kLoopAnim = "Power_Attack_Loop";//構えたまま溜める
	constexpr float kAnimSpeed = 0.5f;
	constexpr float kStartBlendFrame = 9.0f;

	constexpr float kDecelFrame = 8.0f;//最高速から止まるまでのフレーム数
	constexpr float kDecel = Game::kMoveSpeed / kDecelFrame;//1フレームに減る速さ
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
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, kStartAnim, false, kAnimSpeed, -1.0f, 0.0f, kStartBlendFrame);

}

void PlayerStateCharge::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	UpdateMoveVel(Vector3(0, 0, 0), kDecel);

	//構え始めが終わったらループへ
	if (!m_isLoop && player->m_anim.GetAnimEndFlag())
	{
		m_isLoop = true;
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, kLoopAnim, true, kAnimSpeed);
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
}

void PlayerStateCharge::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Charge %.0f/%.0f", m_frame, kChargeThreshold);
}
