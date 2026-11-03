#include "KingmechMissileTurn.h"
#include "Character/Enemy/Kingmech/BossEnemyKingmech.h"
#include "Character/Player/Player.h"
#include "Main/System.h"

namespace
{
	const std::string kMoveAnim = "Run_Mech";//中央へ移動
	const std::string kFireAnim = "Roar";//ミサイルを撃つ
	const std::string kRecoverAnim = "Idle";//後隙

	constexpr float kMoveSpeed = 16.0f;//中央へ移動する速さ
	constexpr float kArriveDistance = 50.0f;//中央からこの距離に入ったら着いた
	constexpr float kMoveMaxTime = 300.0f;//これだけ移動しても着かなければその場で撃つ

	constexpr int kShotNum = 5;//1セットで撃つ数
	constexpr int kSetNum = 3;//セットの数
	constexpr float kFirstFireTime = 30.0f;//撃ち始めるまで
	constexpr float kFireInterval = 30.0f;//撃つ間隔
	constexpr float kSetRestTime = 90.0f;//セットの間の休憩

	constexpr float kRecoverTime = 90.0f;//後隙の長さ
}

KingmechMissileTurn::KingmechMissileTurn(std::weak_ptr<EnemyBase> owner) :
	EnemyStateBase(owner)
{
}

KingmechMissileTurn::~KingmechMissileTurn()
{
}

void KingmechMissileTurn::Enter()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	owner->SetPreventFall(true);
	ChangePhase(Phase::Move);
}

void KingmechMissileTurn::Update()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	const float dt = System::GetInstance().GetTimeScale() * owner->m_ownTimeScale;
	m_frame += dt;

	switch (m_phase)
	{
	case Phase::Move:
		//着いたか、時間切れなら撃つ
		if (owner->ChaseTarget(owner->m_arenaCenter, kArriveDistance, kMoveSpeed) || m_frame >= kMoveMaxTime)
		{
			owner->m_rb.m_vel = Vector3(0, 0, 0);
			ChangePhase(Phase::Fire);
		}
		break;
	case Phase::Fire:
		owner->ToPlayerLook();
		m_fireTimer -= dt;
		if (m_fireTimer <= 0.0f)
		{
			FireAtPlayer();
			++m_shotCount;
			m_fireTimer = kFireInterval;
			//1セット撃ち終わったら休憩//全部のセットが終わったら待つ
			if (m_shotCount >= kShotNum)
			{
				m_shotCount = 0;
				++m_setCount;
				m_fireTimer = kSetRestTime;
				if (m_setCount >= kSetNum)
				{
					ChangePhase(Phase::Wait);
				}
			}
		}
		break;
	case Phase::Wait:
		owner->ToPlayerLook();
		if (!owner->HasMissile())
		{
			ChangePhase(Phase::Recover);
		}
		break;
	case Phase::Recover:
		if (m_frame >= kRecoverTime)
		{
			owner->ChangeState(owner->NextAfterRecover());
		}
		break;
	}
}

void KingmechMissileTurn::Exit()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	owner->SetPreventFall(false);
}

void KingmechMissileTurn::ChangePhase(Phase next)
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	m_phase = next;
	m_frame = 0.0f;

	switch (m_phase)
	{
	case Phase::Move:
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kMoveAnim), true, owner->GetAnimSpeed(kMoveAnim));
		break;
	case Phase::Fire:
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kFireAnim), true, owner->GetAnimSpeed(kFireAnim));
		m_fireTimer = kFirstFireTime;
		m_shotCount = 0;
		m_setCount = 0;
		break;
	case Phase::Wait:
		break;
	case Phase::Recover:
		owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName(kRecoverAnim), true, owner->GetAnimSpeed(kRecoverAnim));
		break;
	}
}

void KingmechMissileTurn::FireAtPlayer()
{
	auto owner = std::dynamic_pointer_cast<BossEnemyKingmech>(m_owner.lock());
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;

	//プレイヤーの足元//着弾の高さは中央の地面//一旦
	Vector3 landPos = player->GetRigidBody().GetPos();
	landPos.y = owner->m_arenaCenter.y;
	owner->FireMissile(landPos);
}
