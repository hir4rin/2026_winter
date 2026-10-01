#include "PlayerStatePartBrokenKill.h"
#include "Player.h"
#include "../../Enemy/EnemyBase.h"
#include "../../Enemy/State/Hit/EnemyPartBrokenKilled.h"
#include "../../../System.h"
#include "../Camera/CameraManager.h"
#include "../../../Math/Easing.h"
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

	//確殺のパターン分岐の条件の敵とプレイヤーの距離
	constexpr float kPatternSelectDistance = 180.0f;



	const std::string kAttackStartName = "root|Combo_Attack_01_01";
	const float kAttackAnimEndFrame = 23.07f;//アニメーション倍率をかける

	//パターンAのStart中のタイムスケールイージング(min->max->min)
	constexpr float kTimeScaleMin = 0.3f;
	constexpr float kTimeScaleMax = 0.9f;
	constexpr float kTimeScalePeakRate = 0.5f;//Startのアニメ進行度のどこで最大になるか(0~1)
	constexpr float kTimeScalePower = 2.0f;

	//パターンAのExecute中のタイムスケールイージング(1.0->kExecuteTimeScaleEnd)
	constexpr float kPatternAHeadBrokenAnimFrame = 37.0f;//頭が取れるアニメフレーム(Assasin04)
	constexpr float kExecuteTimeScaleEnd = 0.4f;
	constexpr float kExecuteTimeScalePower = 2.0f;

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


	//パターン分岐//もっと細かく分けてもいい
	SelectedPattern();

	//確殺対象を固定する
	player->SetIsPartBrokenKilling(true);

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
		player->m_anim.Init(player->m_modelHandle, kAttackStartName.c_str(), false, 0.7f);
		//スタートする
		m_state = PartBrokenKill::Start;

		//敵のステートを変える
		partBrokenTarget->OnPartBrokenKilled(CharacterBase::PartBrokenPattern::A);

		//カメラを切り替える
		auto cameraManager = player->m_cameraManager.lock();
		if (!cameraManager)return;
		cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::PartBrokenACameraStart);

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
		cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::PartBrokenBCameraStart);
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

	//確殺対象の固定を解除する
	player->SetIsPartBrokenKilling(false);
	//ターゲットをリセット
	player->ClearPartBrokenTarget();

	//タイムスケールを戻し忘れないようにする
	System::GetInstance().SetTimeScale(1.0f);
}

void PlayerStatePartBrokenKill::DebugDraw()
{
	auto player = m_owner.lock();
	if (!player) return;

}

void PlayerStatePartBrokenKill::SelectedPattern()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto partBrokenTarget = player->GetPartBrokenTarget();
	if (!partBrokenTarget)return;

	float distance = (player->GetRigidBody().GetPos() - partBrokenTarget->GetRigidBody().GetPos()).Magnitude();

	if (distance < kPatternSelectDistance)
	{
		//近距離
		m_pattern = CharacterBase::PartBrokenPattern::A;
	}
	else
	{
		//遠距離
		m_pattern = CharacterBase::PartBrokenPattern::B;
	}

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
		//Startのアニメの進行度(0~1)でタイムスケールをイージングする 0.3->0.9->0.3
		{
			float p = std::clamp(player->m_anim.GetNowAnimFrame() / kAttackAnimEndFrame, 0.0f, 1.0f);
			float scale;
			if (p < kTimeScalePeakRate)
			{
				float e = Easing::Apply(EasingMode::EaseOut, p / kTimeScalePeakRate, kTimeScalePower);
				scale = kTimeScaleMin + (kTimeScaleMax - kTimeScaleMin) * e;
			}
			else
			{
				float e = Easing::Apply(EasingMode::EaseIn, (p - kTimeScalePeakRate) / (1.0f - kTimeScalePeakRate), kTimeScalePower);
				scale = kTimeScaleMax + (kTimeScaleMin - kTimeScaleMax) * e;
			}
			System::GetInstance().SetTimeScale(scale);
		}

		if (player->m_anim.GetNowAnimFrame() >= kAttackAnimEndFrame)
		{
			m_state = PartBrokenKill::Execute;
			player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Assasin04"), false, 1.0f);
			System::GetInstance().SetTimeScale(1.0f);
			cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::PartBrokenACamera);
		}

	}
	break;
	case PartBrokenKill::Execute:
		//実行中
		//ちょこっとだけ動かす
		//player->m_rb.m_vel = toPlayer * -1 * kExecuteSpeed;
		m_excuteTimer += 1.0f * System::GetInstance().GetTimeScale();
		{
			//頭が取れるフレームまでのアニメ進行度(0~1)でタイムスケールをイージングする 1.0->0.4
			float p = std::clamp(player->m_anim.GetNowAnimFrame() / kPatternAHeadBrokenAnimFrame, 0.0f, 1.0f);
			float e = Easing::Apply(EasingMode::EaseIn, p, kExecuteTimeScalePower);
			System::GetInstance().SetTimeScale(1.0f + (kExecuteTimeScaleEnd - 1.0f) * e);
		}
		if (player->m_anim.GetNowAnimFrame() >= kPatternAHeadBrokenAnimFrame)
		{
			m_state = PartBrokenKill::End;
			System::GetInstance().SetTimeScale(1.0f);
			cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::PartBrokenACameraEnd);
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
			cameraManager->ChangeStateFromScene(CameraManager::CameraStateName::PartBrokenBCamera);

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
