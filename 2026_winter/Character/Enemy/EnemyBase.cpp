#include "EnemyBase.h"
#include "Enemy/State/General/EnemyStateBase.h"
#include "Player.h"
#include "../../Game.h"
#include "../System.h"

namespace
{
	constexpr float kCautionMoveSpeedRate = 0.5f;//警戒移動時の速度倍率
	constexpr float kCautionBackVecRate = 0.2f;//半径を維持するためのベクトルの倍率

	constexpr float kEnemyMeleeAttackRange = 400.0f;//敵の近接攻撃の距離
	constexpr float kEnemyBackDistance = 600.0f;//敵が距離を取るときの距離

	constexpr int kStateChangeRandomMax = 100;//Chase/Caution遷移の抽選範囲
	constexpr int kStateChangeThreshold = 50;//Chase/Caution遷移のしきい値

	constexpr float kToTargetPower = 3.0f;//プレイヤーの正面に行くようにknockBackする力


	constexpr float kEnemyDistance = 50.0f;

}

EnemyBase::EnemyBase(std::weak_ptr<Player> player)
	: m_player(player)
{
}

EnemyBase::~EnemyBase()
{
}

void EnemyBase::OnCollision(Collider& other)
{
}

void EnemyBase::OnDamage(Collider& other, AttackData& data)
{
	auto player = m_player.lock();
	if (!player)return;

	//データの保存
	m_attackData = data;

	//死亡していたら処理しない
	if (m_isDead)return;
	//死亡吹っ飛び中は処理しない
	if (m_isDieOut)return;
	//Playerの攻撃データをもとに被ダメ処理をする
	m_hp -= static_cast<int>(data.attackPower);

	////ダメージがあるなら、ヒットエフェクトを再生する//必殺技の時は、ヒットエフェクトをスローのものにする
	//if (static_cast<int>(data.attackPower) > 0)
	//{
	//	//必殺技
	//	if (data.attackPower >= kUltDamagePower)
	//	{
	//		//m_hitEfPlayingHandle = PlayEffekseer3DEffectSlow(m_hitEfHandle, 0.5f);
	//	}
	//	//その他
	//	else
	//	{
	//		m_hitEfPlayingHandle = PlayEffekseer3DEffect(m_hitEfHandle);
	//		SetPosPlayingEffekseer3DEffect(m_hitEfPlayingHandle, m_pos.x, m_pos.y + kEnemyEfOffset, m_pos.z);
	//	}
	//}

	//if (m_hp <= 0)
	//{
	//	//空中じゃ死なない
	//	if (!IsFloor())
	//	{
	//		m_hp = 1;
	//	}
	//	else
	//	{
	//		m_hp = 0;
	//		m_isLifeZero = true;
	//		//当たり判定を解除する
	//		Terminate();

	//		//キリモミ吹っ飛びの時は、途中で死ぬ
	//		if (m_attackData.isKirimomi)
	//		{
	//			m_isDieOut = true;
	//		}
	//		//死亡アニメーションに移行
	//		else
	//		{
	//			ChangeState(EnemyState::Dead);
	//			return;
	//		}
	//	}
	//}

	//Enemy->Playerのベクトルに吹き飛ばす力を加える//プレイヤーの正面に行くようにknockBackする//いずれkirimomi吹っ飛びの時の処理と分ける
	//吸着させる
	Vector3 front = player->GetTargetVec();
	Vector3 pos = player->GetRigidBody().GetPos();
	Vector3 TargetPos = pos + front * kEnemyDistance;
	Vector3 toTarget = (TargetPos - m_rb.m_pos).Normalize() * kToTargetPower;

	Vector3 pushBackVec = (m_rb.m_pos - other.GetRigidBody().GetPos()).Normalize() *
		data.knockBackPower.x;
	pushBackVec += toTarget;

	//ヒット情報の作成
	HitInfo hitinfo = {
		.knockBackVel = Vector3(pushBackVec.x,data.knockBackPower.y,pushBackVec.z),//Y軸の上下降はここで加える
		.duration = data.knockBackFrame,
		.isKirimomi = data.isKirimomi,
	};

	//Stateの切り替え//敵を吹き飛ばす攻撃かどうかで切り替える

	if (data.knockBackPower.y > 0.0f && !hitinfo.isKirimomi)
	{
		//上昇の時の攻撃
		ChangeState(std::make_shared<EnemyHitAir>(GetWeakPtr(), hitinfo));
		return;
	}
	else
		if (data.knockBackPower.y < 0.0f || hitinfo.isKirimomi)
		{
			//下降時の攻撃、もしくは吹っ飛び時の攻撃
			ChangeState(std::make_shared<EnemyHitDrop>(GetWeakPtr(), hitinfo));
			return;
		}
		else
		{
			//普通の攻撃
			ChangeState(std::make_shared<EnemyHitGround>(GetWeakPtr(), hitinfo));
		}

}

void EnemyBase::ApplyPos()
{
	CharacterBase::ApplyPos();
	//歩いて地面から離れたら落下ステートにする
	if (IsLeftFloor())
	{
		ChangeState(std::make_shared<EnemyAirFall>(GetWeakPtr()));
	}
}

Vector3 EnemyBase::TargetPlayerPos()
{
	if (auto player = m_player.lock())
	{
		return player->GetRigidBody().GetPos();
	}

	return Vector3();
}

bool EnemyBase::ChasePlayer(Vector3 target, float distance)
{
	//プレイヤーの位置に向かって移動する//Y軸は移動しない
	target.y = 0.0f;
	//プレイヤーの手前側が目的地になるように移動//内積は今回はしない
	Vector3 toPlayer = target - m_rb.m_pos;//プレイヤーへのベクトル
	toPlayer.y = 0.0f;

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

void EnemyBase::CautionMove(Vector3 target, float distance)
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

bool EnemyBase::CountInterval(float& timer, float interval)
{
	float timeScale = System::GetInstance().GetTimeScale();

	timer += timeScale * m_ownTimeScale;
	if (timer >= interval)
	{
		timer = 0.0f;
		return true;
	}
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

std::shared_ptr<EnemyStateBase> EnemyBase::NextAfterIdle()
{
	//ランダムでChaseかCautionに遷移する
	if (CanMeleeAttack(kEnemyMeleeAttackRange))
	{
		//return ChangeState(EnemyState::Attack);

		return 	std::make_shared<EnemyAttack>(GetWeakPtr());;
	}
	//ランダム
	if (rand() % kStateChangeRandomMax < kStateChangeThreshold)
	{
		//ChangeState(EnemyState::Chase);
		return std::make_shared<EnemyChase>(GetWeakPtr());

	}
	else if (rand() % kStateChangeRandomMax >= kStateChangeThreshold)
	{
		//ChangeState(EnemyState::Caution);
		return std::make_shared<EnemyCaution>(GetWeakPtr());
	}

	return std::make_shared<EnemyChase>(GetWeakPtr());
}

void EnemyBase::ChangeState(std::shared_ptr<EnemyStateBase> newState)
{
	//現在の状態から抜ける
	if (m_currentState)
	{
		m_currentState->Exit();
	}
	//newStateに更新
	m_prevState = m_currentState;
	m_currentState = newState;
	//newStateの初期化
	if (m_currentState)
	{
		m_currentState->Enter();
	}
}
