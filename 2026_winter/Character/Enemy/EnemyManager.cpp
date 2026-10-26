#include "EnemyManager.h"
#include "EnemyBase.h"
#include "Character/Enemy/Swordman/EnemySwordman.h"
#include "Character/Enemy/Gunner/EnemyGunner.h"
#include "Character/Enemy/Bazooka/EnemyBazooka.h"
#include "Character/Enemy/BossPirate/BossEnemyPirateR1.h"
#include "Character/Enemy/BossPirate/BossEnemyPirateR0.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Battle/AttackTokenManager.h"
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

	//デバッグ用//F3を押した瞬間に、敵のデバッグ表示(頭の上の文字・向きの線)を切り替える
	bool isF3Pressed = CheckHitKey(KEY_INPUT_F3) != 0;
	if (isF3Pressed && !m_wasF3Pressed)
	{
		m_isDebugDraw = !m_isDebugDraw;
	}
	m_wasF3Pressed = isF3Pressed;


	for (auto& enemy : m_enemies)
	{
		//新しく出た敵や、被弾から戻った敵にもかかるように毎フレーム設定する
		enemy->SetDebugIdle(m_isDebugIdle);
		enemy->Update();
	}
	//グループの敵がプレイヤーを見つけたら、同じグループの敵も気づく
	UpdateGroupAlert();

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
		m_totalKillCount++;
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
}

void EnemyManager::DebugDraw()
{
	if (!m_isDebugDraw)return;
	for (auto& enemy : m_enemies)
	{
		enemy->DebugDraw();
	}

	//攻撃権の使用数と攻撃間隔
	if (auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock())
	{
		DrawFormatString(10, 130, GetColor(255, 0, 255), "AttackToken:%d/%d Interval:%.0f",
			tokenMgr->GetUsedCost(), tokenMgr->GetMaxCost(), (std::max)(tokenMgr->GetGlobalInterval(), 0.0f));
	}
}

void EnemyManager::DrawDebugWindows()
{
	for (auto& enemy : m_enemies)
	{
		if (!enemy->HasDebugWindow())continue;
		if (enemy->GetIsVanishing())continue;//消えている死体は出さない
		enemy->DrawDebugWindow();
	}
}

void EnemyManager::SpawnEnemy()
{

	auto enemy = std::make_shared<EnemySwordman>(m_player,Vector3());
	enemy->Init();
	enemy->SetPatrolRoute(0);//パトロールルート0を設定
	enemy->SetIsPlayerFound(false);
	enemy->SetPatrolState();
	m_enemies.push_back(enemy);
}

void EnemyManager::StartPhase(int phase)
{
	m_phaseEnemies.clear();

	for (const auto& data : m_spawnData)
	{
		if (data.phase != phase)continue;
		auto enemy = CreateEnemy(data);
		if (!enemy)continue;
		m_enemies.push_back(enemy);
		m_phaseEnemies.push_back(enemy);
		//ボスの腕など//親の後ろに並べて、親の後に更新する//フェーズの全滅判定には入れない
		for (auto& child : enemy->CreateChildEnemies())
		{
			m_enemies.push_back(child);
		}
	}
}

void EnemyManager::DebugClearEnemies()
{
	m_enemies.clear();
	m_corpses.clear();
	m_phaseEnemies.clear();
}

bool EnemyManager::IsPhaseEnemiesAllDead() const
{
	//消えた敵は倒されたものとして扱う
	for (const auto& weakEnemy : m_phaseEnemies)
	{
		auto enemy = weakEnemy.lock();
		if (enemy && !enemy->GetIsLifeZero())return false;
	}
	return true;
}

std::shared_ptr<EnemyBase> EnemyManager::CreateEnemy(const EnemySpawnData& data)
{
	std::shared_ptr<EnemyBase> enemy;
	switch (data.type)
	{
	case EnemyType::Swordman:
		enemy = std::make_shared<EnemySwordman>(m_player, data.position);
		break;
	case EnemyType::Gunner:
		enemy = std::make_shared<EnemyGunner>(m_player, data.position);
		break;
	case EnemyType::Bazooka:
		enemy = std::make_shared<EnemyBazooka>(m_player, data.position);
		break;
	case EnemyType::BossEnemyPirateR1:
		enemy = std::make_shared<BossEnemyPirateR1>(m_player, data.position);
		break;
	case EnemyType::BossEnemyPirateR0:
		enemy = std::make_shared<BossEnemyPirateR0>(m_player, data.position);
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
		if (data.route.empty()) enemy->SetWatchPoint();
		else enemy->SetPatrolPoints(data.route);
		enemy->SetIsPlayerFound(false);
		enemy->SetPatrolState();
		break;
	case EnemyInitialState::Watch:
		enemy->SetWatchPoint();
		enemy->SetIsPlayerFound(false);
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

bool EnemyManager::IsGroupAllDead(const std::string& groupId) const
{
	for (const auto& enemy : m_enemies)
	{
		if (enemy->GetGroupId() != groupId)continue;
		if (!enemy->GetIsLifeZero())return false;
	}
	return true;
}
