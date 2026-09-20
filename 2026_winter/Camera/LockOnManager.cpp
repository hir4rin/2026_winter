#include "LockOnManager.h"
#include "../Managers/CollisionManager.h"
#include "../Character/Enemy/EnemyBase.h"
#include <cassert>


LockOnManager::LockOnManager()
{
}

LockOnManager::~LockOnManager()
{
}

void LockOnManager::SetTargetEnemy(int id)
{
	//idで指定したEnemyをターゲットにする
	auto enemy = CollisionManager::GetInstance().GetColliderById(id);
	if (!enemy)
	{
		//assert(false && "指定したidのEnemyが存在しません");
		return;
	}
	//EnemyBaseのshared_ptrを取得
	auto enemyBase = std::dynamic_pointer_cast<EnemyBase>(enemy);
	if (!enemyBase)
	{
		//assert(false && "指定したidのEnemyはEnemyBaseではありません");
		return;
	}

	//セット
	m_targetEnemy = enemyBase;

	//m_context->m_targetEnemy = enemyBase;
	//for (auto& camera : m_weakRefCameras)
	//{
	//	camera->SetCameraContext(m_context);
	//}
	////mainCameraにもセット
	//m_mainCamera->SetCameraContext(m_context);

}

std::weak_ptr<EnemyBase> LockOnManager::GetTarget()
{
	auto target = m_targetEnemy.lock();
	if (target)return m_targetEnemy;

	return {};
}
