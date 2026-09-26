#pragma once
#include <memory>
#include <vector>
#include "../Math/Vector3.h"


class EnemyBase;
class Player;
class EnemyManager;
class CameraManager;
class CaemeraManager;


class LockOnManager
{
public:
	LockOnManager();
	virtual ~LockOnManager();

	//弱参照をセット
	void Init(std::weak_ptr<Player> player, std::weak_ptr<CameraManager> cameraManager, std::weak_ptr<EnemyManager> enemyManager);

	//入力(ロックオン、ロックオン切り替え)、敵の死亡、距離のチェックをする
	void Update();

	bool IsLockOn()const { return !m_lockTarget.expired(); }//まだいるときはfalseが返ってくる
	std::shared_ptr<EnemyBase> GetLockTarget()const { return m_lockTarget.lock(); }



	
private:
	//必殺技、ラストヒット中はfalseを返す
	bool CanOperate()const;
	void TryLockOn();
	//ロックオンしている敵の解放
	void Release();
	//dir:右が正、左が負
	void Switch(int dir);
	//死亡なら次の敵へ、離れすぎたら解除
	void CheckTarget();



	//カメラの座標と向きを取得
	bool GetCameraInfo(Vector3& pos, Vector3& dir)const;
	//範囲内の生きている敵だけを集める//第二引数は除外する敵
	std::vector<std::shared_ptr<EnemyBase>> CollectEnemiesRange(float range, const std::shared_ptr<EnemyBase>& exclude = nullptr)const;
	//fromからdir方向に一番近い敵を返す(cosで判定)//candidatesは候補者
	std::shared_ptr<EnemyBase> PickByCos(const Vector3& from, const Vector3& dir, const std::vector<std::shared_ptr<EnemyBase>>& candidates)const;


private:
	std::weak_ptr<Player> m_player;
	std::weak_ptr<CameraManager> m_cameraManager;
	std::weak_ptr<EnemyManager> m_enemyManager;

	std::weak_ptr<EnemyBase> m_lockTarget = {};//ロックオンしている対象

};

