#include "EnemyManager.h"
#include "EnemyBase.h"
#include "EnemySwordman.h"
#include "Player.h"
#include "../Input.h"
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <string>

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

	//デバッグ用//Iを押した瞬間に、敵をずっとIdleにするかを切り替える
	bool isIPressed = CheckHitKey(KEY_INPUT_I) != 0;
	if (isIPressed && !m_wasIPressed)
	{
		m_isDebugIdle = !m_isDebugIdle;
	}
	m_wasIPressed = isIPressed;


	for (auto& enemy : m_enemies)
	{
		//新しく出た敵や、被弾から戻った敵にもかかるように毎フレーム設定する
		enemy->SetDebugIdle(m_isDebugIdle);
		enemy->Update();
	}
	//グループの敵がプレイヤーを見つけたら、同じグループの敵も気づく
	UpdateGroupAlert();
	//フェーズの更新//今のフェーズの敵が全員倒されたら、次のフェーズを始める
	UpdatePhase();

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

void EnemyManager::StartSpawn(const std::vector<EnemySpawnData>& spawnData)
{
	m_spawnData = spawnData;
	m_currentPhase = 0;
	m_phaseEnemies.clear();

	int firstPhase = FindNextPhase();
	if (firstPhase < 0)return;//配置データが無い
	StartPhase(firstPhase);
}

void EnemyManager::StartPhase(int phase)
{
	m_currentPhase = phase;
	m_phaseEnemies.clear();

	for (const auto& data : m_spawnData)
	{
		if (data.phase != phase)continue;
		auto enemy = CreateEnemy(data);
		if (!enemy)continue;
		m_enemies.push_back(enemy);
		m_phaseEnemies.push_back(enemy);
	}
}

void EnemyManager::UpdatePhase()
{
	if (m_currentPhase <= 0)return;//配置データで始めていない

	//今のフェーズの敵が1体でも生きていたら、まだ次へ進まない
	for (const auto& weakEnemy : m_phaseEnemies)
	{
		auto enemy = weakEnemy.lock();
		if (enemy && !enemy->GetIsLifeZero())return;
	}

	int nextPhase = FindNextPhase();
	if (nextPhase < 0)
	{
		//最後のフェーズを倒しきった//何度もここを通らないように、敵のリストだけ空にしておく
		m_phaseEnemies.clear();
		return;
	}
	StartPhase(nextPhase);
}

int EnemyManager::FindNextPhase() const
{
	int next = -1;
	for (const auto& data : m_spawnData)
	{
		if (data.phase <= m_currentPhase)continue;
		if (next < 0 || data.phase < next) next = data.phase;
	}
	return next;
}

std::shared_ptr<EnemyBase> EnemyManager::CreateEnemy(const EnemySpawnData& data)
{
	std::shared_ptr<EnemyBase> enemy;
	switch (data.type)
	{
	case EnemyType::Swordman:
		enemy = std::make_shared<EnemySwordman>(m_player, data.position);
		break;
	default:
		return nullptr;
	}
	enemy->Init();//ここでIdleになる
	enemy->SetFacing(data.rotYDeg);
	enemy->SetGroupId(data.groupId);

	switch (data.initialState)
	{
	case EnemyInitialState::Idle:
		break;//Initで設定済み
	case EnemyInitialState::Patrol:
		//ルートが無ければ、その場で見張る
		if (data.route.empty()) enemy->SetGuardPoint();
		else enemy->SetPatrolPoints(data.route);
		enemy->SetPatrolState();
		break;
	case EnemyInitialState::Guard:
		enemy->SetGuardPoint();
		enemy->SetPatrolState();
		break;
	}
	return enemy;
}

void EnemyManager::UpdateGroupAlert()
{
	//プレイヤーを見つけた敵がいるグループを集める//暗殺した場合も対象にするかも
	std::unordered_set<std::string> alertedGroups;
	for (const auto& enemy : m_enemies)
	{
		if (enemy->GetGroupId().empty())continue;
		if (!enemy->GetIsPlayerFound())continue;
		//insertは同じグループIDがあっても重複しない
		//insertは文字列をunordersetの箱に入れると

		alertedGroups.insert(enemy->GetGroupId());
	}
	if (alertedGroups.empty())return;

	//同じグループの敵に気づかせる
	for (auto& enemy : m_enemies)
	{
		if (enemy->GetGroupId().empty())continue;
		if (!alertedGroups.contains(enemy->GetGroupId()))continue;
		enemy->OnAlerted();
	}
}
