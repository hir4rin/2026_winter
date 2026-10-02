#include "LockOnManager.h"
#include "../Managers/CollisionManager.h"
#include "../Character/Enemy/EnemyBase.h"
#include "../Character/Enemy/EnemyManager.h"
#include "Player.h"
#include "CameraManager.h"
#include "CameraState/CameraStateBase.h"
#include "../BattleManager.h"
#include "../System.h"
#include "../Input.h"
#include <cassert>

namespace
{
	constexpr float kLockOnMaxDistance = 10000.0f;//これ以上離れたらロックオンを解除する
	constexpr int kLockOnCheckNum = 3;//ロックオン対象を探すとき、範囲を広げる回数
	constexpr float kLockOnRangeExpandMult = 3.0f;//切り替え・再取得時の範囲の倍率

	constexpr float kDebugSphereRadius = 20.0f;//デバッグ描画の球の半径
	constexpr float kDebugSphereOffsetY = 200.0f;//デバッグ描画の球の高さ
	constexpr int kDebugSphereDivNum = 8;//デバッグ描画の球の分割数
}


LockOnManager::LockOnManager()
{
}

LockOnManager::~LockOnManager()
{
}

void LockOnManager::Init(std::weak_ptr<Player> player, std::weak_ptr<CameraManager> cameraManager, std::weak_ptr<EnemyManager> enemyManager)
{
	m_player = player;
	m_cameraManager = cameraManager;
	m_enemyManager = enemyManager;
}
void LockOnManager::Update()
{
	auto& input = Input::GetInstance();


	//暗殺周り
	AssasinUpdate();

	//確殺周り
	PartBrokenUpdate();

	//必殺技、ラストヒット中はreturn;
	//ロックオンの処理-------------------------------------
	if (!CanOperate())return;
	

	//ターゲットの距離・死んでいるかのチェック
	CheckLockOnTarget();

	if (input.IsTriggered("RB"))
	{
		//ロックオン中だったら
		if (IsLockOn())
		{
			Release();
		}
		else
		{
			TryLockOn();
		}
		return;
	}

	//右スティックでロックオン対象を切り替える
	if (IsLockOn() && input.IsTriggeredRightStickInputX())
	{

		int dir = input.GetRightStickInput().x > 0.0f ? 1 : -1;

		//取得した方向側の敵を取得
		Switch(dir);
		return;
	}

	

}

bool LockOnManager::CanOperate() const
{
	auto battleMgr = System::GetInstance().GetBattleMgr();
	if (!battleMgr)return true;
	if (battleMgr->GetIsUltimating())return false;
	if (battleMgr->GetIsLastHitEventPlaying())return false;
	return true;
}

void LockOnManager::TryLockOn()
{
	auto player = m_player.lock();
	if (!player)return;
	Vector3 cameraPos, cameraDir;
	//カメラの座標とカメラレイ
	if (!GetCameraInfo(cameraPos, cameraDir))return;

	//範囲内にいる敵を見つける
	std::vector<std::shared_ptr<EnemyBase>> candidates;
	//範囲をだんだん広げる
	for (int i = 1; i <= kLockOnCheckNum; i++)
	{
		candidates = CollectEnemiesRange(player->GetCameraRockOnRange() * i);
		if (!candidates.empty())break;
	}

	//画面の中央にいる敵をロックオン
	auto best = PickByCos(cameraPos, cameraDir, candidates);
	if (!best)return;

	m_lockTarget = best;

	//ロックオン中は内部ターゲットを使わないので、消す
	player->ClearSoftTarget();
}

void LockOnManager::Release()
{
	m_lockTarget.reset();
}

void LockOnManager::Switch(int dir)
{
	auto target = m_lockTarget.lock();
	auto player = m_player.lock();
	if (!target)return;
	if (!player)return;
	Vector3 cameraPos, cameraDir;
	if (!GetCameraInfo(cameraPos, cameraDir))return;

	//候補者を探す
	auto candidates = CollectEnemiesRange(player->GetCameraRockOnRange() * kLockOnRangeExpandMult, target);

	//カメラから今のターゲットへのベクトル
	Vector3 toTarget = (target->GetRigidBody().GetPos() - cameraPos).Normalize();
	//外積によって右方向を出す
	Vector3 rightVec = (toTarget.Cross(Vector3(0.0f, 1.0f, 0.0f)) * -1.0f).Normalize();

	//入力下側にいる敵のうち角度が小さい敵を選ぶ//Cosで判定
	std::shared_ptr<EnemyBase> best = nullptr;
	float minSide = 1.0f;
	for (auto& enemy : candidates)
	{
		Vector3 toEnemy = (enemy->GetRigidBody().GetPos() - cameraPos).Normalize();
		//カメラの後ろの敵はのぞいたっていい
		if (toTarget.Dot(toEnemy) <= 0.0f)continue;
		//入力の向きを掛けることで、どちらも対応できるようにする
		float side = rightVec.Dot(toEnemy) * static_cast<float>(dir);
		if (side <= 0.0f)continue;
		if (side < minSide)
		{
			minSide = side;
			best = enemy;
		}
	}
	if (best)m_lockTarget = best;


}

void LockOnManager::CheckLockOnTarget()
{
	auto target = m_lockTarget.lock();
	auto player = m_player.lock();
	if (!target)return;
	if (!player)return;

	//離れすぎたら解除
	float distance = (target->GetRigidBody().GetPos() - player->GetRigidBody().GetPos()).Magnitude();
	if (distance > kLockOnMaxDistance)
	{
		//解除
		Release();
		return;
	}
	//生きていれば早期リターン
	if (!target->GetIsLifeZero())return;

	//死んでいたら、死んだ敵の方向に一番近い敵へ切り替える//いなければ解除
	Vector3 cameraPos, cameraDir;
	if (!GetCameraInfo(cameraPos, cameraDir))
	{
		Release();
		return;
	}

	auto candidates = CollectEnemiesRange(player->GetCameraRockOnRange() * kLockOnRangeExpandMult, target);

	Vector3 toTarget = (target->GetRigidBody().GetPos() - cameraPos).Normalize();
	auto next = PickByCos(cameraPos, toTarget, candidates);
	if (next)
	{
		m_lockTarget = next;
	}
	else
	{
		Release();
		return;
	}


}

void LockOnManager::TryAssasinTarget()
{
	auto player = m_player.lock();
	if (!player)return;
	Vector3 cameraPos, cameraDir;
	//カメラの座標とカメラレイ
	if (!GetCameraInfo(cameraPos, cameraDir))return;

	//細かい範囲の調整は後で調整

	//範囲内にいる敵を見つける
	std::vector<std::shared_ptr<EnemyBase>> candidates;
	//範囲をだんだん広げる
	for (int i = 1; i <= kLockOnCheckNum; i++)
	{
		candidates = CollectEnemiesRange(player->GetCameraRockOnRange() * i);
		//処刑済み(確殺・暗殺が始まった)敵、空中にいる敵、プレイヤーを発見している敵は外す//範囲を広げる判定より前に外す
		std::erase_if(candidates, [](const auto& enemy)
		{
			return enemy->GetIsExecuted() || !enemy->IsFloor() || enemy->GetIsPlayerFound();
		});
		if (!candidates.empty())break;
	}

	//画面の中央にいる敵をロックオン
	auto best = PickByCos(cameraPos, cameraDir, candidates);
	if (!best)return;

	//playerの暗殺対象にセット
	player->SetAssasinTarget(best);
}

void LockOnManager::CheckAssasinTarget()
{
	auto player = m_player.lock();
	if (!player)return;
	auto target = player->GetAssasinTarget();
	if (!target)return;

	//処刑済みなら解除(確殺で倒された敵が暗殺対象に残らないようにする)
	if (target->GetIsExecuted())
	{
		player->ClearAssasinTarget();
		return;
	}

	//空中にいる敵は解除
	if (!target->IsFloor())
	{
		player->ClearAssasinTarget();
		return;
	}

	//プレイヤーを発見した敵は解除(暗殺対象にした後で見つかった場合)
	if (target->GetIsPlayerFound())
	{
		player->ClearAssasinTarget();
		return;
	}

	//細かい範囲の調整は後で調整


	//離れすぎたら解除
	float distance = (target->GetRigidBody().GetPos() - player->GetRigidBody().GetPos()).Magnitude();
	if (distance > kLockOnMaxDistance)
	{
		//解除
		player->ClearAssasinTarget();
		return;
	}
	//生きていれば早期リターン
	if (!target->GetIsLifeZero())return;

	//死んでいたら、解除
	Vector3 cameraPos, cameraDir;
	if (!GetCameraInfo(cameraPos, cameraDir))
	{
		player->ClearAssasinTarget();
		return;
	}
}

void LockOnManager::AssasinUpdate()
{
	//暗殺演出中は対象を固定する(途中で別の敵に切り替わらないようにする)
	auto player = m_player.lock();
	if (!player)return;
	if (player->GetIsAssasinating())return;

	//暗殺対象を探す
	TryAssasinTarget();
	//暗殺対象を外すかチェック
	CheckAssasinTarget();
}

void LockOnManager::TryPartBrokenTarget()
{
	auto player = m_player.lock();
	if (!player)return;
	Vector3 cameraPos, cameraDir;
	//カメラの座標とカメラレイ
	if (!GetCameraInfo(cameraPos, cameraDir))return;

	//細かい範囲の調整は後で調整

	//範囲内にいる敵を見つける
	std::vector<std::shared_ptr<EnemyBase>> candidates;
	//範囲をだんだん広げる
	for (int i = 1; i <= kLockOnCheckNum; i++)
	{
		candidates = CollectEnemiesRange(player->GetCameraRockOnRange() * i);
		//部位破壊している、かつ処刑済みでない、かつ地上にいる敵のみにする//範囲を広げる判定より前に外す
		std::erase_if(candidates, [](const auto& enemy)
		{
			return !enemy->GetIsPartBroken() || enemy->GetIsExecuted() || !enemy->IsFloor();
		});
		if (!candidates.empty())break;
	}


	//画面の中央にいる敵をロックオン
	auto best = PickByCos(cameraPos, cameraDir, candidates);
	if (!best)return;

	//playerの暗殺対象にセット
	player->SetPartBrokenTarget(best);
}

void LockOnManager::CheckPartBrokenTarget()
{
	auto player = m_player.lock();
	if (!player)return;
	auto target = player->GetPartBrokenTarget();
	if (!target)return;

	//処刑済みなら解除(暗殺で倒された敵が確殺対象に残らないようにする)
	if (target->GetIsExecuted())
	{
		player->ClearPartBrokenTarget();
		return;
	}

	//空中にいる敵は解除
	if (!target->IsFloor())
	{
		player->ClearPartBrokenTarget();
		return;
	}

	//細かい範囲の調整は後で調整


	//離れすぎたら解除
	float distance = (target->GetRigidBody().GetPos() - player->GetRigidBody().GetPos()).Magnitude();
	if (distance > kLockOnMaxDistance)
	{
		//解除
		player->ClearPartBrokenTarget();
		return;
	}
	//生きていれば早期リターン
	if (!target->GetIsLifeZero())return;

	//死んでいたら、解除
	Vector3 cameraPos, cameraDir;
	if (!GetCameraInfo(cameraPos, cameraDir))
	{
		player->ClearPartBrokenTarget();
		return;
	}
}

void LockOnManager::PartBrokenUpdate()
{
	//確殺演出中は対象を固定する(途中で別の敵に切り替わらないようにする)
	auto player = m_player.lock();
	if (!player)return;
	if (player->GetIsPartBrokenKilling())return;

	//確殺対象を探す
	TryPartBrokenTarget();
	//確殺対象を外すかチェック
	CheckPartBrokenTarget();
}

bool LockOnManager::GetCameraInfo(Vector3& pos, Vector3& dir) const
{
	auto cameraManager = m_cameraManager.lock();
	if (!cameraManager)return false;
	auto camera = cameraManager->GetActiveCamera();
	if (!camera)return false;

	pos = camera->GetPos();
	dir = (camera->GetTarget() - pos).Normalize();
	return true;
}

std::vector<std::shared_ptr<EnemyBase>> LockOnManager::CollectEnemiesRange(float range, const std::shared_ptr<EnemyBase>& exclude) const
{
	std::vector<std::shared_ptr<EnemyBase>> RangeEnemies;
	auto player = m_player.lock();
	auto enemyManager = m_enemyManager.lock();
	if (!player)return RangeEnemies;
	if (!enemyManager)return RangeEnemies;

	Vector3 playerPos = player->GetRigidBody().GetPos();
	for (auto enemy : enemyManager->GetEnemies())
	{
		if (!enemy)continue;
		//同じだったら除外
		if (enemy == exclude)continue;
		if (enemy->GetIsLifeZero())continue;

		float distance = (enemy->GetRigidBody().GetPos() - playerPos).Magnitude();
		//範囲内の敵をすべて入れる
		if (distance < range)
		{
			RangeEnemies.push_back(enemy);
		}
	}
	return RangeEnemies;
}

std::shared_ptr<EnemyBase> LockOnManager::PickByCos(const Vector3& from, const Vector3& dir, const std::vector<std::shared_ptr<EnemyBase>>& candidates) const
{
	std::shared_ptr<EnemyBase> best = nullptr;
	float maxCos = -1.0f;
	for (auto& enemy : candidates)
	{
		Vector3 toEnemy = (enemy->GetRigidBody().GetPos() - from).Normalize();
		float cos = dir.Dot(toEnemy);
		if (!best || cos > maxCos)
		{
			maxCos = cos;
			best = enemy;
		}
	}
	return best;
}

