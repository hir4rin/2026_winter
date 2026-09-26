#include "EnemyManager.h"
#include "EnemyBase.h"
#include "Player.h"

EnemyManager::EnemyManager()
{
}

EnemyManager::~EnemyManager()
{
}

void EnemyManager::AddEnemy(std::shared_ptr<EnemyBase> enemy)
{
	if (!enemy)return;
	m_enemies.push_back(enemy);
}

void EnemyManager::Update()
{
	for (auto& enemy : m_enemies)
	{
		enemy->Update();
	}
}

void EnemyManager::Draw()
{
	for (auto& enemy : m_enemies)
	{
		enemy->Draw();
	}
}
