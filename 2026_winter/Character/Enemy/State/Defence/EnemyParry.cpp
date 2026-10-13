#include "EnemyParry.h"
#include "Main/System.h"
#include "../../EnemyBase.h"
#include "Character/Player/Player.h"
#include <format>

namespace
{
	constexpr float kParryAnimRate = 0.5f;

	constexpr float kAnimEndFrame = 27.0f;
	constexpr float kAnimMoveEndFrame = 20.0f;
}

EnemyParry::EnemyParry(std::weak_ptr<EnemyBase> owner) :
	EnemyStateBase(owner)
{
}

EnemyParry::~EnemyParry()
{
}

void EnemyParry::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//TODO:弾き用のアニメーションが決まったら差し替える//仮でガードで受けるアニメーション
	owner->m_anim.ChangeAnim(owner->GetAnimName("BlockHit"), false, kParryAnimRate);

	//その場で止まり、プレイヤーの方を向く
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	owner->ToPlayerLook();

	//ガードゲージを0に戻す
	owner->m_guardInfo.GurardGauge = 0.0f;

	m_timer = 0.0f;

	//プレイヤーに弾かれたことを伝えてひるませる(ダメージ・無敵時間の処理は通さない)
	if (auto player = owner->m_player.lock())
	{
		player->OnParried();
	}
}

void EnemyParry::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//押し戻しの処理が続かないように毎フレーム消す
	owner->m_rb.m_vel = Vector3(0, 0, 0);

	m_timer += System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	if (m_timer >= owner->m_status.parryTime)
	{
		owner->ChangeState(owner->NextAfterParry());
		return;
	}
}

void EnemyParry::Exit()
{
}

std::string EnemyParry::GetDebugInfo() const
{
	auto owner = m_owner.lock();
	if (!owner)return{};

	return std::format("{:.0f}/{:.0f}F", m_timer, owner->m_status.parryTime);
}
