#include "PlayerStateAssasin.h"
#include "Player.h"
#include "../../Enemy/EnemyBase.h"
#include "../../../System.h"


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
}

PlayerStateAssasin::PlayerStateAssasin(std::weak_ptr<Player> player) :PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする
	if (m_owner.expired())return;
}

PlayerStateAssasin::~PlayerStateAssasin()
{
}

void PlayerStateAssasin::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;

	//暗殺名前一覧
	//	Execute01->Execute01Victim     移動させないとださい　△
	//	Execute02->Execute02Victim　　まあまあ　　　採用
	//	Execute03->Execute03Victim   一瞬
	//	Execute04->FootPlantVictim   一瞬そこそこ　　絶技採用
	//	Assasin01->RunSlashFinisher　ちょっと地味　移動ぎり感すごい
	//	Assasin02->fromAvobeForwardVictim 地上からなら微妙
	//	Assasin03->StabBehindVictim　まあまあ　でも終わりポーズがださい
	//	Assasin04->StabChestVictim   わりかし


	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("Execute02"), false, 0.7f);
	//スタートする
	m_state = AssasinState::Start;

	//敵のステートを変える
	auto assasinTarget = player->GetAssasinTarget();
	if (!assasinTarget)return;
	assasinTarget->OnAssasined();

	//キャラ同士の押し戻しを有効化
	player->SetIsGhost(true);

	//敵のほうを向く
	Vector3 toEnemy = assasinTarget->GetRigidBody().GetPos() - player->GetRigidBody().GetPos();
	player->m_targetVec = toEnemy.Normalize();
}

void PlayerStateAssasin::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto assasinTarget = player->GetAssasinTarget();
	if (!assasinTarget)return;

	Vector3 playerPos = player->GetRigidBody().GetPos();
	Vector3 enemyPos = assasinTarget->GetRigidBody().GetPos();

	Vector3 toPlayer = playerPos - enemyPos;
	toPlayer = toPlayer.Normalize();
	toPlayer.y = 0;

	switch (m_state)
	{
	case AssasinState::Start:
	{
		//敵のそばまでlerpで近寄る
		Vector3 targetPos = enemyPos + toPlayer * kStartDistance;

		m_startTimer += 1.0f * System::GetInstance().GetTimeScale();
		float t = m_startTimer / kStartMaxTimer;

		player->m_rb.m_pos = Vector3::Lerp(playerPos, targetPos, t);
		if (m_startTimer > kStartMaxTimer)
		{
			m_state = AssasinState::Execute;
		}

	}
	break;
	case AssasinState::Execute:
		//実行中
		//ちょこっとだけ動かす
		player->m_rb.m_vel = toPlayer * -1 * kExecuteSpeed;
		m_excuteTimer += 1.0f * System::GetInstance().GetTimeScale();
		if (m_excuteTimer > kExecuteTimer)
		{
			m_state = AssasinState::End;
		}

		break;
	case AssasinState::End:
		break;
	}


	if (player->m_anim.GetAnimEndFlag())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));//Idle状態に遷移する
		return;
	}
	player->m_anim.Update();
}

void PlayerStateAssasin::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;

	//キャラ同士の押し戻しを有効化
	player->SetIsGhost(false);
}

void PlayerStateAssasin::DebugDraw()
{
}
