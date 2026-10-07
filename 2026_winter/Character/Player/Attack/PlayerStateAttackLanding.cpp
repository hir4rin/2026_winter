#include "PlayerStateAttackLanding.h"
#include "Player.h"
#include "../../../Input.h"
#include "../../../System.h"



//ここのフレームの部分は後ほど調整
namespace
{
	//弱攻撃(空中弱攻撃の最終段)の着地硬直
	constexpr float kLightDodgeCancelFrame = 6.0f;//回避でキャンセルできるフレーム
	constexpr float kLightActionCancelFrame = 15.0f;//攻撃・ジャンプ・確殺・移動でキャンセルできるフレーム

	//強攻撃(空中強攻撃)の着地硬直
	constexpr float kHeavyDodgeCancelFrame = 15.0f;//回避でキャンセルできるフレーム
	constexpr float kHeavyActionCancelFrame = 25.0f;//攻撃・ジャンプ・確殺・移動でキャンセルできるフレーム
	constexpr float kHeavyLandingAnimTimeScale = 1.6f;//着地モーションの再生速度
	constexpr float kHeavyLandingAnimEndFrame = 19.0f;//着地モーションの終了フレーム
}

PlayerStateAttackLanding::PlayerStateAttackLanding(std::weak_ptr<Player> player, AttackType type) :
	PlayerState(player), m_attackType(type)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateAttackLanding::~PlayerStateAttackLanding()
{
}

void PlayerStateAttackLanding::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//攻撃の種類によって着地モーションと硬直フレームを変える
	if (m_attackType == AttackType::heavyAttack)
	{
		m_dodgeCancelFrame = kHeavyDodgeCancelFrame;
		m_actionCancelFrame = kHeavyActionCancelFrame;
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("HeavyAttackLanding"), false, kHeavyLandingAnimTimeScale, kHeavyLandingAnimEndFrame);
	}
	else if (m_attackType == AttackType::lightAttack)
	{
		m_dodgeCancelFrame = kLightDodgeCancelFrame;
		m_actionCancelFrame = kLightActionCancelFrame;
		//空中弱攻撃の最終段の着地モーションはなし
	}

	//硬直中は動かない
	player->m_rb.m_vel = Vector3(0, 0, 0);
	m_landingTimer = 0.0f;
}

void PlayerStateAttackLanding::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//硬直中は動かない
	player->m_rb.m_vel = Vector3(0, 0, 0);

	//硬直時間はアニメーションのフレームではなくタイマーで数える(アニメーションより硬直が長いため)
	m_landingTimer += 1.0f * System::GetInstance().GetTimeScale();

	//回避
	if (CanDodgeCancel() && input.IsTriggered("B"))
	{
		player->ChangeState(std::make_shared<PlayerStateDodge>(m_owner));
		return;
	}

	if (CanActionCancel())
	{
		//確殺
		if (input.IsTriggered("Y") && player->CanPartBrokenFinish())
		{
			player->ChangeState(std::make_shared<PlayerStatePartBrokenKill>(m_owner));
			return;
		}
		//ジャンプ
		if (input.IsTriggered("A"))
		{
			player->ChangeState(std::make_shared<PlayerStateJump>(m_owner));
			return;
		}
		//弱攻撃
		if (input.IsTriggered("X"))
		{
			player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::lightAttack));
			return;
		}
		//強攻撃
		if (input.IsTriggered("Y"))
		{
			player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::heavyAttack));
			return;
		}
		//移動
		if (input.IsLeftStickInput())
		{
			player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
			return;
		}
	}

	//硬直が終わったらIdleに戻る
	if (CanActionCancel())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
		return;
	}

	//アニメーションの更新
	player->m_anim.Update();
}

void PlayerStateAttackLanding::Exit()
{
	//おそらく落下攻撃の後の吹き飛ばしがtrueになり、isHitがtrueになって次の初段ガ動かないので、falseにする
	auto player = m_owner.lock();
	if (!player)return;
	player->m_comboInfo.isHit = false;
}

void PlayerStateAttackLanding::DebugDraw()
{
	auto player = m_owner.lock();
	if (!player) return;
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:AttackLanding(%s)", m_attackType == AttackType::heavyAttack ? "Heavy" : "Light");
	DrawFormatString(10, 30, GetColor(255, 255, 255), "LandingTimer:%.1f (Dodge:%.1f Action:%.1f)", m_landingTimer, m_dodgeCancelFrame, m_actionCancelFrame);
	DrawFormatString(10, 50, GetColor(255, 255, 255), "DodgeCancel:%d ActionCancel:%d", CanDodgeCancel(), CanActionCancel());
}

bool PlayerStateAttackLanding::CanDodgeCancel()
{
	return m_landingTimer >= m_dodgeCancelFrame;
}

bool PlayerStateAttackLanding::CanActionCancel()
{
	return m_landingTimer >= m_actionCancelFrame;
}
