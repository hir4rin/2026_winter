#include "PlayerStatePartBrokenKill.h"
#include "Player.h"
#include "../../Enemy/EnemyBase.h"
#include "../../Enemy/State/Hit/EnemyPartBrokenKilled.h"
#include "../../../System.h"
#include "../Camera/CameraManager.h"
#include <algorithm>

namespace
{
	//スタート時にたどり着く敵までの距離
	constexpr float kStartDistance = 50.0f;
	//最初のStartのフレーム
	constexpr float kStartMaxTimer = 20.0f;
	//実行中の動く速度
	constexpr float kExecuteSpeed = 8.0f;
	//実行中のフレーム
	constexpr float kExecuteTimer = 25.0f;

	const std::string kAttackStartName = "root|Combo_Attack_01_01";
	const float kAttackAnimEndFrame = 23.07f;//アニメーション倍率をかける

	//パターンB----
	// 最初はイースインアウト
	//めっっちゃすすむフレーム21.68
	//そのあとピタッととまる　39
	constexpr float kStartMoveFrame = 21.68f;
	constexpr float kEndMoveFrame = 39.0f;
	//実行中の動く速度(パターンB)
	constexpr float kExecuteSpeedB = 20.0f;

}

PlayerStatePartBrokenKill::PlayerStatePartBrokenKill(std::weak_ptr<Player> player):PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStatePartBrokenKill::~PlayerStatePartBrokenKill()
{
}

void PlayerStatePartBrokenKill::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	auto partBrokenTarget = player->GetPartBrokenTarget();
	if (!partBrokenTarget)return;

	m_pattern = CharacterBase::PartBrokenPattern::B;

	//キャラ同士の押し戻しを有効化
	player->SetIsGhost(true);

	//敵のほうを向く
	Vector3 toEnemy = partBrokenTarget->GetRigidBody().GetPos() - player->GetRigidBody().GetPos();
	toEnemy.y = 0.0f;
	player->m_targetVec = toEnemy.Normalize();


	//暗殺名前一覧
	//	Execute01->Execute01Victim     移動させないとださい　△
	//	Execute02->Execute02Victim　　まあまあ　　　採用
	//	Execute03->Execute03Victim   一瞬
	//	Execute04->FootPlantVictim   一瞬そこそこ　　いいけどかぶってる感あるから　一旦見送り
	//	Assasin01->RunSlashFinisher　ちょっと地味　移動ぎり感すごい   確殺採用
	//	Assasin02->fromAvobeForwardVictim 地上からなら微妙
	//	Assasin03->StabBehindVictim　まあまあ　でも終わりポーズがださい
	//	Assasin04->StabChestVictim   わりかし  確殺採用

	switch (m_pattern)
	{
	case CharacterBase::PartBrokenPattern::A:
	{
		//player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Assasin04"), false, 1.0f);
		player->m_anim.Init(player->m_modelHandle, kAttackStartName.c_str(), false, 1.0f);
		//スタートする
		m_state = PartBrokenKill::Start;

		//敵のステートを変える
		partBrokenTarget->OnPartBrokenKilled(CharacterBase::PartBrokenPattern::A);

		//カメラを切り替える
		auto cameraManager = player->m_cameraManager.lock();
		if (!cameraManager)return;
		//cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::AssasinCameraStart);

		//System::GetInstance().SetTimeScale(0.7f);
	}
		break;
	case CharacterBase::PartBrokenPattern::B:
	{
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Assasin01"), false, 1.0f);
		//スタートする
		m_state = PartBrokenKill::Start;

		//敵のステートを変える
		partBrokenTarget->OnPartBrokenKilled(CharacterBase::PartBrokenPattern::B);

		//カメラを切り替える
		auto cameraManager = player->m_cameraManager.lock();
		if (!cameraManager)return;
		//cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::AssasinCameraStart);
		//System::GetInstance().SetTimeScale(0.7f);
	}
		break;
	}

	
}

void PlayerStatePartBrokenKill::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto partBrokenTarget = player->GetPartBrokenTarget();
	if (!partBrokenTarget)return;
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;

	switch (m_pattern)
	{
	case CharacterBase::PartBrokenPattern::A:
		PatternAUpdate();
		break;
	case CharacterBase::PartBrokenPattern::B:
		PatternBUpdate();
		break;
	}

	player->m_anim.Update();
}

void PlayerStatePartBrokenKill::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;

	//キャラ同士の押し戻しを有効化
	player->SetIsGhost(false);
}

void PlayerStatePartBrokenKill::DebugDraw()
{
}

void PlayerStatePartBrokenKill::PatternAUpdate()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto partBrokenTarget = player->GetPartBrokenTarget();
	if (!partBrokenTarget)return;
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = partBrokenTarget->GetRigidBody().GetPos();

	Vector3 toPlayer = playerPos - enemyPos;
	toPlayer = toPlayer.Normalize();
	toPlayer.y = 0;

	switch (m_state)
	{
	case PartBrokenKill::Start:
	{
		//敵のそばまでlerpで近寄る
		Vector3 targetPos = enemyPos + toPlayer * kStartDistance;

		m_startTimer += 1.0f * System::GetInstance().GetTimeScale();
		float t = m_startTimer / kStartMaxTimer;

		player->m_rb.m_pos = Vector3::Lerp(playerPos, targetPos, t);
		//if (m_startTimer > kStartMaxTimer)
		//{
		//	m_state = PartBrokenKill::Execute;
		//	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Assasin04"), false, 1.0f);

		//	//cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::AssasinCamera);
		//	//System::GetInstance().SetTimeScale(0.7f);
		//}
		if (player->m_anim.GetNowAnimFrame() >= kAttackAnimEndFrame)
		{
			m_state = PartBrokenKill::Execute;
			player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Assasin04"), false, 1.0f);
		}

	}
	break;
	case PartBrokenKill::Execute:
		//実行中
		//ちょこっとだけ動かす
		//player->m_rb.m_vel = toPlayer * -1 * kExecuteSpeed;
		m_excuteTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_excuteTimer > kExecuteTimer)
		{
			m_state = PartBrokenKill::End;
			//System::GetInstance().SetTimeScale(1.0f);
			//cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::PlayerCaemra);
		}

		break;
	case PartBrokenKill::End:
		break;
	}


	if (player->m_anim.GetAnimEndFlag())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
		return;
	}
}

void PlayerStatePartBrokenKill::PatternBUpdate()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto partBrokenTarget = player->GetPartBrokenTarget();
	if (!partBrokenTarget)return;
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = partBrokenTarget->GetRigidBody().GetPos();

	Vector3 toPlayer = playerPos - enemyPos;
	toPlayer = toPlayer.Normalize();
	toPlayer.y = 0;

	switch (m_state)
	{
	case PartBrokenKill::Start:
	{
		//敵のそばまでlerpで近寄る
		Vector3 targetPos = enemyPos + toPlayer * kStartDistance;

		m_startTimer += 1.0f * System::GetInstance().GetTimeScale();
		float t = m_startTimer / kStartMaxTimer;
		t = std::clamp(t, 0.0f, 1.0f);

		player->m_rb.m_pos = Vector3::EaseLerp(playerPos, targetPos, t,EasingMode::EaseInOut,2.0f);

		//if (m_startTimer > kStartMaxTimer)
		//{
		//	m_state = PartBrokenKill::Execute;
		//	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Assasin04"), false, 1.0f);

		//	//cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::AssasinCamera);
		//	//System::GetInstance().SetTimeScale(0.7f);
		//}

		if (player->m_anim.GetNowAnimFrame() >= kStartMoveFrame)
		{
			m_state = PartBrokenKill::Execute;
		}

	}
	break;
	case PartBrokenKill::Execute:
		//実行中
		//急スピード
		player->m_rb.m_vel = player->m_targetVec * kExecuteSpeedB;
		m_excuteTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (player->m_anim.GetNowAnimFrame() >= kEndMoveFrame)
		{
			m_state = PartBrokenKill::End;
			//System::GetInstance().SetTimeScale(1.0f);
			//cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::PlayerCaemra);
		}

		break;
	case PartBrokenKill::End:
		break;
	}


	if (player->m_anim.GetAnimEndFlag())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
		return;
	}

}
