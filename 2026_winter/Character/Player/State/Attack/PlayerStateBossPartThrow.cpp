#include "PlayerStateBossPartThrow.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBase.h"

namespace
{
	constexpr int kThrowComboIndex = ComboIndex::AirAttack5;//投げのモーションは空中弱攻撃5と同じ
	constexpr float kDefaultThrowRate = 0.4f;//攻撃判定のフレームが無いときに投げる進行率

	constexpr float kThrowFrame = 17.2f;

	constexpr float kEndAnimAttackFrame = 22.0f;
}

PlayerStateBossPartThrow::PlayerStateBossPartThrow(std::weak_ptr<Player> player) :
	PlayerState(player)
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//パーツを切り落としたボスをプレイヤーから受け取る
	m_target = owner->m_bossPartThrowTarget;
	owner->m_bossPartThrowTarget.reset();
}

PlayerStateBossPartThrow::~PlayerStateBossPartThrow()
{
}

void PlayerStateBossPartThrow::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	const ComboNode& node = player->m_comboChain[kThrowComboIndex];
	//ここは投げは途中スロウとか速くするとかのアニメスピードをイースインアウトがいいと思った
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, node.animName, false, 0.3f, node.endFrame, 0.0f, node.blendFrame);
	player->m_anim.SetRootMotionEnable(RootMotionCancel::Down);//空中で止まったまま投げる

	//弱5の攻撃判定が出るフレームで投げる
	//m_throwFrame = node.attackColStartFrames.empty() ?
	//	player->m_anim.GetAnimEndFrame() * kDefaultThrowRate : node.attackColStartFrames[0];
	m_throwFrame = kThrowFrame;
	m_isThrown = false;
}

void PlayerStateBossPartThrow::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	//空中で止まる
	player->m_rb.m_vel = Vector3(0, 0, 0);

	//ボスの方を向く
	if (auto target = m_target.lock())
	{
		Vector3 toTarget = target->GetRigidBody().GetPos() - player->m_rb.m_pos;
		toTarget.y = 0.0f;
		if (toTarget.Magnitude() > 0.0f)player->m_targetVec = toTarget.Normalize();
	}

	//投げる
	if (!m_isThrown && player->m_anim.GetNowAnimFrame() >= m_throwFrame)
	{
		ThrowPart();
	}

	//モーションが終わったら落下
	if (player->m_anim.GetNowAnimFrame() >= kEndAnimAttackFrame)
	{
		player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));
		return;
	}

	player->m_anim.Update();
}

void PlayerStateBossPartThrow::Exit()
{
	//途中で抜けても投げておく(ボスが待ち続けないように)
	ThrowPart();
}

void PlayerStateBossPartThrow::DebugDraw()
{
	auto player = m_owner.lock();
	if (!player) return;
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:BossPartThrow %.0f/%.0f %s",
		player->m_anim.GetNowAnimFrame(), m_throwFrame, m_isThrown ? "Thrown" : "Hold");
}

void PlayerStateBossPartThrow::ThrowPart()
{
	if (m_isThrown)return;
	m_isThrown = true;
	if (auto target = m_target.lock())target->ThrowPart();
}
