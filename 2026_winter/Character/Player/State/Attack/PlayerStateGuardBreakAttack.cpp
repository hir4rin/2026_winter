#include "PlayerStateGuardBreakAttack.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/System.h"
#include "Character/Enemy/EnemyBase.h"

namespace
{
	constexpr float kDefaultComboInputStartRate = 0.2f;//入力受付開始
	constexpr float kDefaultComboInputEndRate = 0.8f;//入力受付終了
	constexpr float kDefaultCancelRate = 0.5f;//次の攻撃へ

	constexpr float kEndWaitFrame = 20.0f;//切り上げ後の待ち
}

PlayerStateGuardBreakAttack::PlayerStateGuardBreakAttack(std::weak_ptr<Player> player) :
	PlayerStateAttackBase(player)
{
	if (m_owner.expired())return;
}

PlayerStateGuardBreakAttack::~PlayerStateGuardBreakAttack()
{
}

void PlayerStateGuardBreakAttack::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//落下禁止を有効化
	player->SetPreventFall(true);
	DecideAttackDirection();

	player->m_isCameraFocusOverride = false;
	player->m_cameraFocusEnemy.reset();

	//TODO: 専用カメラ・演出

	StartPhase(Phase::Skill0);
}

void PlayerStateGuardBreakAttack::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	AttackMoveMent();

	switch (m_phase)
	{
	case Phase::Skill0:
		UpdateSkill0();
		break;
	case Phase::UpAttack:
		UpdateUpAttack();
		break;
	}
}

void PlayerStateGuardBreakAttack::Exit()
{
	auto player = m_owner.lock();
	if (player)
	{
		//落下禁止を無効化
		player->SetPreventFall(false);
		player->m_comboInfo.currentComboIndex = ComboIndex::None;
		player->m_comboInfo.isHit = false;
		player->m_isCameraFocusOverride = false;
		player->m_bossPartThrowTarget.reset();//投げに行かずに抜けたとき用(投げのステートは作るときに受け取っている)
	}

	//TODO: 専用カメラ・演出の終了

	ReleaseAttackCol();
}

void PlayerStateGuardBreakAttack::DebugDraw()
{
	auto player = m_owner.lock();
	if (!player) return;
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:GuardBreakAttack %s",
		m_phase == Phase::Skill0 ? "Skill0" : "UpAttack");
	DrawFormatString(10, 30, GetColor(255, 255, 255), "ComboIndex:%d", player->m_comboInfo.currentComboIndex);
	DrawFormatString(10, 50, GetColor(255, 255, 255), "isHit:%d", player->m_comboInfo.isHit);
}

void PlayerStateGuardBreakAttack::StartPhase(Phase phase)
{
	auto player = m_owner.lock();
	if (!player) return;

	ReleaseAttackCol();//前の段の判定

	m_phase = phase;
	m_nextAttackType = AttackType::None;

	int comboIndex = (phase == Phase::Skill0) ? ComboIndex::GuardBreakAttack0 : ComboIndex::GuardBreakAttack1;
	player->m_comboInfo.currentComboIndex = comboIndex;
	player->m_comboInfo.isHit = false;

	const ComboNode& node = player->m_comboChain[comboIndex];
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame, 0.0f, node.blendFrame);
	//上昇はmoveSpeedYでする
	if (phase == Phase::UpAttack)
	{
		player->m_anim.SetRootMotionEnable(RootMotionCancel::Up);
	}

	InitVerticalMove(node);
	CreateAttackCol(node);
}

void PlayerStateGuardBreakAttack::UpdateSkill0()
{
	auto player = m_owner.lock();
	if (!player) return;

	//UpdateLaunchCamera();

	const ComboNode& node = player->m_comboChain[player->m_comboInfo.currentComboIndex];
	float nowFrame = player->m_anim.GetNowAnimFrame();
	float cancelFrame = ResolveTransitionFrame(node.cancelFrame, kDefaultCancelRate);

	//当たっていたら切り上げへ
	if (nowFrame >= cancelFrame)
	{
		if (player->m_comboInfo.isHit)
		{
			StartPhase(Phase::UpAttack);
			return;
		}
		ToLocomotion();
		return;
	}

	player->m_anim.Update();
}

void PlayerStateGuardBreakAttack::UpdateUpAttack()
{
	auto player = m_owner.lock();
	if (!player) return;

	//ボスのパーツを切り落としたら、切り上げの後は自動で投げ返す
	if (!player->m_bossPartThrowTarget.expired())
	{
		if (player->m_anim.GetAnimEndFlag())
		{
			player->ChangeState(std::make_shared<PlayerStateBossPartThrow>(m_owner));
			return;
		}
		player->m_anim.Update();
		return;
	}

	AttackInputCheck();

	const ComboNode& node = player->m_comboChain[player->m_comboInfo.currentComboIndex];
	float nowFrame = player->m_anim.GetNowAnimFrame();
	float cancelFrame = ResolveTransitionFrame(node.cancelFrame, kDefaultCancelRate);

	//空中コンボへ
	if (nowFrame >= cancelFrame && m_nextAttackType != AttackType::None)
	{
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, m_nextAttackType));
		return;
	}

	if (player->m_anim.GetAnimEndFlag())
	{
		player->m_rb.m_vel.y = 0.0f;
		//少し待ってから
		m_endTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_endTimer < kEndWaitFrame)return;

		if (player->IsFloor())
		{
			ToLocomotion();
			return;
		}
		player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));
		return;
	}

	player->m_anim.Update();
}

void PlayerStateGuardBreakAttack::UpdateLaunchCamera()
{
	auto player = m_owner.lock();
	if (!player) return;
	if (player->m_isCameraFocusOverride)return;
	if (!player->m_comboInfo.isHit)return;
	auto enemy = player->GetAttackTarget();
	if (!enemy)return;
	player->m_cameraFocusEnemy = enemy;
	player->m_isCameraFocusOverride = true;
}

void PlayerStateGuardBreakAttack::AttackInputCheck()
{
	auto player = m_owner.lock();
	if (!player) return;
	if (m_nextAttackType != AttackType::None)return;

	const ComboNode& node = player->m_comboChain[player->m_comboInfo.currentComboIndex];
	float inputStartFrame = ResolveTransitionFrame(node.comboInputStartFrame, kDefaultComboInputStartRate);
	float inputEndFrame = ResolveTransitionFrame(node.comboInputEndFrame, kDefaultComboInputEndRate);
	float nowFrame = player->m_anim.GetNowAnimFrame();
	if (nowFrame < inputStartFrame || nowFrame > inputEndFrame)return;

	auto& input = Input::GetInstance();
	if (input.IsPressed("LB"))return;
	if (input.IsTriggered("X"))m_nextAttackType = AttackType::lightAttack;
	else if (input.IsTriggered("Y"))m_nextAttackType = AttackType::heavyAttack;
}

void PlayerStateGuardBreakAttack::ToLocomotion()
{
	auto player = m_owner.lock();
	if (!player) return;
	if (Input::GetInstance().IsLeftStickInput())
	{
		player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
	}
	else
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
	}
}
