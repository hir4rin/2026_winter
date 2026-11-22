#include "PlayerStateEndingJump.h"
#include "Character/Player/Player.h"
#include "Battle/BattleManager.h"
#include "Main/System.h"
#include <cmath>

namespace
{
	constexpr float kLiftOffFrame = 0.0f;//爆風の中からすぐ飛び出す
	const char* const kLeapPoseAnim = "Dodge_Combat_F";//飛び出す姿勢(スキル血殺の分身と同じ)
	constexpr float kLeapPoseFrame = 13.0f;
	constexpr float kFallAnimFrame = 40.0f;//落ちるアニメに変える(頂点を過ぎたあたり)
	constexpr float kOutSpeed = 20.0f;//横へ跳ぶ速さ
	constexpr float kUpSpeed = 30.0f;//上へ跳ぶ速さ//車両の端に着く前に床より十分高くなるように
	constexpr float kGravity = 0.9f;
	constexpr float kBackAccel = 0.35f;//列車に置いていかれる(後ろへの加速)
	constexpr float kBlendFrame = 4.0f;
	constexpr float kFallBlendFrame = 20.0f;//姿勢から落ちるアニメへゆっくり
}

PlayerStateEndingJump::PlayerStateEndingJump(std::weak_ptr<Player> player, const Vector3& outDir, const Vector3& backDir) :
	PlayerState(player),
	m_outDir(outDir),
	m_backDir(backDir)
{
}

PlayerStateEndingJump::~PlayerStateEndingJump()
{
}

void PlayerStateEndingJump::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//列車の外へ出るので当たり判定を切る
	player->SetIsActive(false);
	player->m_rb.m_vel = Vector3(0, 0, 0);
	player->m_rb.m_moveVel = Vector3(0, 0, 0);
	player->m_targetVec = m_outDir;
	player->SetRotY(atan2f(m_outDir.x, m_outDir.z) - DX_PI_F);
	m_startPos = player->m_rb.m_pos;
	SetPos(*player, m_startPos);

	//飛び出す姿勢で止める
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, kLeapPoseAnim, false, 0.0f, -1.0f, kLeapPoseFrame, kBlendFrame);
	m_frameCount = 0.0f;
	m_isFallAnim = false;
}

void PlayerStateEndingJump::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	//タイムスケールで進める(飛び出す瞬間のスローに合わせる)
	m_frameCount += System::GetInstance().GetTimeScale();
	player->m_rb.m_vel = Vector3(0, 0, 0);

	if (!m_isFallAnim && m_frameCount >= kFallAnimFrame)
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("JumpDown"), true, 1.0f, -1.0f, 0.0f, kFallBlendFrame);
		m_isFallAnim = true;
	}

	if (m_frameCount > kLiftOffFrame)
	{
		const float t = m_frameCount - kLiftOffFrame;
		const Vector3 up = Vector3(0.0f, kUpSpeed * t - 0.5f * kGravity * t * t, 0.0f);
		SetPos(*player, m_startPos + m_outDir * (kOutSpeed * t) + up + m_backDir * (0.5f * kBackAccel * t * t));
	}

	//足の白いもやと肩の風の筋(ダッシュと同じもの)
	player->SetFootTrailEmitting(true);
	player->SetShoulderTrailEmitting(true);

	player->m_anim.Update();
}

void PlayerStateEndingJump::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->SetIsActive(true);
}

void PlayerStateEndingJump::SetPos(Player& player, const Vector3& pos)
{
	//当たり判定を切っている間はApplyPosが呼ばれないので、モデルもここで動かす
	player.m_rb.m_pos = pos;
	player.UpdateAngleAndPos();
}
