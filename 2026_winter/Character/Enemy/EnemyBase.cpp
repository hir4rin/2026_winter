#include "EnemyBase.h"
#include "Player.h"
#include "../../Game.h"

namespace
{
	constexpr float kCautionMoveSpeedRate = 0.5f;//警戒移動時の速度倍率
	constexpr float kCautionBackVecRate = 0.2f;//半径を維持するためのベクトルの倍率
}

EnemyBase::EnemyBase(std::weak_ptr<Player> player)
	: m_player(player)
{
}

EnemyBase::~EnemyBase()
{
}

std::string EnemyBase::GetEnemyStateString(EnemyState state)
{
	std::string ans = "";
	switch (state)
	{
		case EnemyState::Idle:
		ans = "Idle";
		break;
		case EnemyState::Caution:
			ans = "Caution";
			break;
		case EnemyState::Chase:
			ans = "Chase";
			break;
		case EnemyState::Attack:
			ans = "Attack";
			break;
		case EnemyState::Back:
			ans = "Back";
			break;
		case EnemyState::Hit:
			ans = "Hit";
			break;
		case EnemyState::AirStay:
			ans = "AirStay";
			break;
		case EnemyState::Fall:
			ans = "Fall";
			break;
		default : 
			ans = "Unknown";
			break;
	}
	return ans;
}

Vector3 EnemyBase::TargetPlayerPos()
{
	if (auto player = m_player.lock())
	{
		return player->GetRigidBody().GetPos();
	}

	return Vector3();
}

bool EnemyBase::ChasePlayer(Vector3 target,float distance)
{
	//プレイヤーの位置に向かって移動する//Y軸は移動しない
	target.y = 0.0f;
	//プレイヤーの手前側が目的地になるように移動//内積は今回はしない
	Vector3 toPlayer = target - m_rb.m_pos;//プレイヤーへのベクトル

	//playerとの距離を図り、手前側かつ、指定距離まで来たら移動を止める
	if (toPlayer.Magnitude() <= distance)
	{
		//目的地に到達した
		m_rb.m_vel = Vector3(0, 0, 0);
		return true;
	}
	//速度を指定
	m_rb.m_vel = toPlayer.Normalize() * Game::kEnemyMoveSpeed;

	return false;

}

void EnemyBase::CautionMove(Vector3 target,float distance)
{
	//プレイヤーの位置と自分の位置から円を描くように移動する//Y軸は移動しない
	//playerからEnemyへのベクトルの接線方向に移動
	Vector3 toEnemy = m_rb.m_pos - target;
	toEnemy.y = 0.0f;
	//接線の求め方はベクトルを90度回転させるので成分を入れ替えて、xを符号反転　
	Vector3 tangentLine = Vector3(-toEnemy.z, 0.0f, toEnemy.x).Normalize();
	m_rb.m_vel = tangentLine * Game::kEnemyMoveSpeed * kCautionMoveSpeedRate;
	//半径を維持するために後ろ側にもベクトルを加える
	float dist = toEnemy.Magnitude();
	if (dist < distance)
	{
		Vector3 backVec = toEnemy.Normalize() * (distance - dist) * kCautionBackVecRate;//半径を維持するためのベクトル//
		m_rb.m_vel += backVec;
	}
}

bool EnemyBase::BackMove(Vector3 target, float distance)
{
	//ToEnemyの方向に移動する
	Vector3 toEnemy = m_rb.m_pos - target;
	toEnemy.y = 0.0f;
	//到達したら移動を止める
	if (toEnemy.Magnitude() >= distance)
	{
		m_rb.m_vel = Vector3(0, 0, 0);
		return true;
	}

	m_rb.m_vel = toEnemy.Normalize() * Game::kEnemyBackSpeed;
	return false;
}

bool EnemyBase::CanMeleeAttack(float distance)
{
	//クールタイムの確認
	if (m_attackCoolTime <= 0.0f)
	{

		//プレイヤーとの距離が遠かったら攻撃しない
		Vector3 toPlayer = TargetPlayerPos() - m_rb.m_pos;
		toPlayer.y = 0.0f;
		if (toPlayer.Magnitude() <= distance)
		{
			return true;
		}
	}
	//攻撃しない
	return false;
}

void EnemyBase::ToPlayerLook()
{
	auto player = m_player.lock();
	if (!player)return;

	Vector3 toPlayer = player->GetRigidBody().GetPos() - m_rb.m_pos;
	m_targetVec = toPlayer.Normalize();
}

void EnemyBase::FinishHitProcess()
{
	//初期化しておく
	//m_knockBackVel = Vector3(0, 0, 0);
	m_knockBackFrame = 0;
	m_hitType = HitType::None;
}
