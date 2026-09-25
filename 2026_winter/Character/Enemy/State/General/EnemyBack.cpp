#include "EnemyBack.h"
#include "../../EnemyBase.h"
#include "Player.h"

namespace
{
	constexpr float kEnemyBackDistance = 600.0f;//敵が距離を取るときの距離
}

EnemyBack::EnemyBack(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyBack::~EnemyBack()
{
}

void EnemyBack::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	auto player = owner->m_player.lock();
	if (!player)return;
	//animationの初期化
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Back"), false);
}

void EnemyBack::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//敵の挙動改良案
	//バックステップの距離をランダムにする
	//確率でバックステップではなくCautionにする

		//一定距離離れたらIdle
	if (owner->BackMove(owner->m_targetPos, kEnemyBackDistance))
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
		return;
	}
}

void EnemyBack::Exit()
{
}
void EnemyBack::DebugDraw()
{
}
