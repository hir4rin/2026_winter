#include "EnemyManager.h"
#include "EnemyBase.h"
#include "EnemySwordman.h"
#include "Player.h"
#include "../Input.h"
#include <memory>
#include <algorithm>

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

	//デバッグ用//Fを押した瞬間だけ敵を1体出す
	bool isFPressed = CheckHitKey(KEY_INPUT_F) != 0;
	if (isFPressed && !m_wasFPressed)
	{
		SpawnEnemy();
	}
	m_wasFPressed = isFPressed;


	for (auto& enemy : m_enemies)
	{
		enemy->Update();
	}

	//死体の数を制限する//m_enemiesを回している途中に消すと壊れるので、Updateの後に行う
	UpdateCorpses();
}

void EnemyManager::UpdateCorpses()
{
	//新しく死んだ敵を、死んだ順に死体リストへ追加する
	for (auto& enemy : m_enemies)
	{
		if (!enemy->GetIsLifeZero())continue;
		if (enemy->GetIsVanishing())continue;
		//すでに登録済みなら追加しない
		if (std::find(m_corpses.begin(), m_corpses.end(), enemy) != m_corpses.end())continue;
		m_corpses.push_back(enemy);
	}

	//上限を超えたら、一番古い死体から消え始めさせる//消えている途中の死体は数に入れない
	while (m_corpses.size() > kMaxCorpseNum)
	{
		m_corpses.front()->StartVanish();
		m_corpses.pop_front();
	}

	//消え終わった敵を削除する
	std::erase_if(m_enemies, [](const std::shared_ptr<EnemyBase>& enemy)
		{
			return enemy->GetIsVanished();
		});
}

void EnemyManager::Draw()
{
	for (auto& enemy : m_enemies)
	{
		enemy->Draw();
	}

#ifdef _DEBUG
	//敵のステートのデバッグ描画
	for (auto& enemy : m_enemies)
	{
		enemy->DebugDraw();
	}
#endif
}

void EnemyManager::SpawnEnemy()
{

	auto enemy = std::make_shared<EnemySwordman>(m_player,Vector3());
	enemy->Init();
	enemy->SetPatrolRoute(0);//パトロールルート0を設定
	enemy->SetPatrolState();
	m_enemies.push_back(enemy);
}
