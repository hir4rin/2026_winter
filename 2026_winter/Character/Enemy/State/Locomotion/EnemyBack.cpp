#include "EnemyBack.h"
#include "../../EnemyBase.h"
#include "Character/Player/Player.h"

namespace
{
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
	//落下禁止を有効化
	owner->SetPreventFall(true);
	auto player = owner->m_player.lock();
	if (!player)return;
	//animationの初期化
	owner->m_anim.ChangeAnimWithModelHandle(owner->m_modelHandle, owner->GetAnimName("Back"), false, owner->GetAnimSpeed("Back"));
}

void EnemyBack::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//敵の挙動改良案
	//バックステップの距離をランダムにする
	//確率でバックステップではなくCautionにする

		//一定距離離れたらIdle
	if (owner->BackMove(owner->m_targetPos, owner->m_status.backDistance))
	{
		owner->ChangeState(std::make_shared<EnemyIdle>(owner));
		return;
	}
}

void EnemyBack::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下禁止を無効化
	owner->SetPreventFall(false);
}
void EnemyBack::DebugDraw()
{
}
