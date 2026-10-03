#include "EnemyBase.h"
#include "Enemy/State/General/EnemyStateBase.h"
#include "Player.h"
#include "HitCol.h"
#include "../../Game.h"
#include "../System.h"
#include "../../DataLoader/DataManager.h"
#include <algorithm>
#include <cassert>

namespace
{
	//PatrolRouteX.csvの列番号
	enum PatrolRouteColumn : int
	{
		PosX = 0,
		PosY = 1,
		PosZ = 2,
		WaitTime = 3,
		Index = 4,
		Size,//列数
	};

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

void EnemyBase::SetPatrolRoute(int routeId)
{
	//CSVファイルを読み込む
	const auto& rawData = DataManager::GetInstance().GetPatrolRouteRawData();

	m_patrolPoints.clear();//設定し直しのときに前のデータが残らないように消す
	m_currentPatrolIndex = -1;//最初のポイントから巡回し直す

	if (routeId < 0 || routeId >= static_cast<int>(rawData.size()))
	{
		assert(false && "存在しない巡回ルート番号です");
		return;
	}

	for (const auto& tokens : rawData[routeId])
	{
		//列数チェック//tokensは1行分のデータ
		if (tokens.size() < PatrolRouteColumn::Size)
		{
			assert(false && "PatrolRoute.csvの列数が不足しています");
			continue;
		}
		PatrolPoint point;
		point.pos = Vector3(std::stof(tokens[PatrolRouteColumn::PosX]),
			std::stof(tokens[PatrolRouteColumn::PosY]),
			std::stof(tokens[PatrolRouteColumn::PosZ]));
		point.waitTime = std::stof(tokens[PatrolRouteColumn::WaitTime]);
		point.index = std::stoi(tokens[PatrolRouteColumn::Index]);
		m_patrolPoints.push_back(point);
	}

	//CSVの行の順番ではなく、indexの順番で巡回する
	std::sort(m_patrolPoints.begin(), m_patrolPoints.end(),
		[](const PatrolPoint& a, const PatrolPoint& b) { return a.index < b.index; });
}

void EnemyBase::SetFacing(float rotYDeg)
{
	float rad = rotYDeg * DX_PI_F / 180.0f;
	//向きたい方向//Unityと同じで、0度なら+Z、90度なら+Xを向く
	m_targetVec = Vector3(sinf(rad), 0.0f, cosf(rad));
	//UpdateAngleAndPosがm_targetVecに少しずつ近づけるので、最初から向いた状態にしておく
	//(モデルが180度ずれているので、UpdateAngleAndPosと同じくDX_PI_Fずらす)
	m_rotAngleY = rad - DX_PI_F;
}

void EnemyBase::SetPatrolPoints(const std::vector<EnemySpawnRoutePoint>& route)
{
	m_patrolPoints.clear();//設定し直しのときに前のデータが残らないように消す
	m_currentPatrolIndex = -1;//最初のポイントから巡回し直す

	for (int i = 0; i < static_cast<int>(route.size()); ++i)
	{
		PatrolPoint point;
		point.pos = route[i].pos;
		point.waitTime = route[i].waitTime;
		point.index = i;
		m_patrolPoints.push_back(point);
	}
}

void EnemyBase::SetGuardPoint()
{
	//今の位置で、ずっと待機する(waitTimeが負なら次へ出発しない)
	SetPatrolPoints({ { m_rb.m_pos, -1.0f } });
}

void EnemyBase::OnAlerted()
{
	if (m_isPlayerFound)return;//もう気づいている
	if (m_isLifeZero || m_isExecuted)return;
	//巡回・見張り中のときだけ追跡を始める(戦闘中や被弾中のStateは上書きしない)
	if (!std::dynamic_pointer_cast<EnemyPatrol>(m_currentState))return;

	m_isPlayerFound = true;
	ChangeState(std::make_shared<EnemyChase>(GetWeakPtr()));
}

void EnemyBase::OnCollision(Collider& other)
{
}

void EnemyBase::OnDamage(Collider& other, AttackData& data)
{
	auto player = m_player.lock();
	if (!player)return;

	//処刑済み(確殺・暗殺中)なら被弾しない(演出中のStateが上書きされないようにする)
	if (m_isExecuted)return;

	//データの保存
	m_attackData = data;

	//死亡していたら処理しない
	if (m_isDead)return;
	//死亡吹っ飛び中は処理しない
	if (m_isDieOut)return;
	//Playerの攻撃データをもとに被ダメ処理をする
	m_hp -= static_cast<int>(data.attackPower);

	//部位破壊率の確率で部位破壊する//GetRand(99)は0〜99を返す
	if (!m_isPartBroken && GetRand(99) < data.brokenRate)
	{
		OnPartBreak();
	}

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
		.knockBackPowerXZ = data.knockBackPower.x,
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

void EnemyBase::OnAssasined()
{
	//処刑済みにする(もう一度暗殺・確殺の対象にならないようにする)
	m_isExecuted = true;
	//StateをAssasinに変える
	ChangeState(std::make_shared<EnemyAssasined>(GetWeakPtr()));
	return;
}

void EnemyBase::OnPartBrokenKilled(PartBrokenPattern pattern)
{
	//処刑済みにする(もう一度暗殺・確殺の対象にならないようにする)
	m_isExecuted = true;
	//StateをPartBrokenKilledに変える(パターンはコンストラクタで渡す)
	ChangeState(std::make_shared<EnemyPartBrokenKilled>(GetWeakPtr(), pattern));
	return;
}

void EnemyBase::StartVanish()
{
	//すでに消えている途中なら何もしない
	if (m_isVanishing)return;
	ChangeState(std::make_shared<EnemyVanish>(GetWeakPtr()));
}

void EnemyBase::SetOpacity(float rate)
{
	MV1SetOpacityRate(m_modelHandle, rate);
}

void EnemyBase::ApplyPos()
{
	CharacterBase::ApplyPos();
	//歩いて地面から離れたら落下ステートにする//消えている途中の死体は落下ステートにしない
	if (IsLeftFloor() && m_rb.m_vel.y <= 0.0f && !m_isVanishing)
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

bool EnemyBase::ChaseTarget(Vector3 target, float distance,float speed)
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
	m_rb.m_vel = toPlayer.Normalize() * speed;
	//向きを指定
	m_targetVec = toPlayer.Normalize();

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

void EnemyBase::FinisherPerformanceProcess()
{
	m_isDead = true;
	m_isLifeZero = true;
	//やられ判定を消す
	if (m_hitCol)
	{
		m_hitCol->SetIsActive(false);
	}

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
