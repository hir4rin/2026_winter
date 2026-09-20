#pragma once
#include <memory>


class EnemyBase;


class LockOnManager
{
public:
	LockOnManager();
	virtual ~LockOnManager();

	void Init();
	void Update();

	void SetTargetEnemy(std::weak_ptr<EnemyBase> target){ m_targetEnemy = target;}
	void SetTargetEnemy(int id);
	std::weak_ptr<EnemyBase>GetTarget();

	//ロックオン
	void SetLockOn(bool isLockOn) { m_isLockOn = isLockOn; }
	bool GetIsLockOn()const { return m_isLockOn; }
private:
	std::weak_ptr<EnemyBase> m_targetEnemy = {};
	bool m_isLockOn = false;//ロックオンしているかどうか

};

