#pragma once
#include <memory>
#include <vector>
#include <deque>
#include "../../Stage/EnemySpawnData.h"

class EnemyBase;
class Player;

class EnemyManager
{
public:
	EnemyManager();
	virtual ~EnemyManager();

	void AddEnemy(std::shared_ptr<EnemyBase> enemy);//初期化済みの敵を登録する//テストで使う用
	void Update();
	void Draw();


	void SetPlayer(std::weak_ptr<Player> player) { m_player = player; }

	const std::vector<std::shared_ptr<EnemyBase>>& GetEnemies()const { return m_enemies; }

	//敵配置CSV(DataManager::LoadEnemySpawnData)のデータを渡して、最初のフェーズの敵を出す
	//SetPlayerの後に呼ぶこと(敵の生成にプレイヤーが必要なため)
	void StartSpawn(const std::vector<EnemySpawnData>& spawnData);
	int GetCurrentPhase()const { return m_currentPhase; }//今のフェーズ//0なら配置データ無し(まだ始まっていない)
private:
	void SpawnEnemy();
	void UpdateCorpses();//死体の数が上限を超えたら、古い順に消す

	//フェーズ-----------------------------------------------------------------
	void StartPhase(int phase);//そのフェーズの敵をすべて出す
	void UpdatePhase();//今のフェーズの敵が全員倒されたら、次のフェーズを始める
	int FindNextPhase()const;//今のフェーズより大きいフェーズのうち一番小さいもの//無ければ-1
	std::shared_ptr<EnemyBase> CreateEnemy(const EnemySpawnData& data);//配置データ1つ分の敵を作る

	//グループ-----------------------------------------------------------------
	void UpdateGroupAlert();//同じグループの誰かがプレイヤーを見つけていたら、全員に気づかせる
private:
	static constexpr size_t kMaxCorpseNum = 5;//残しておく死体の最大数

	std::vector<std::shared_ptr<EnemyBase>> m_enemies;
	std::deque<std::shared_ptr<EnemyBase>> m_corpses;//死体を死んだ順に並べたもの

	std::weak_ptr<Player> m_player;//プレイヤーの弱参照

	std::vector<EnemySpawnData> m_spawnData;//敵配置のデータ(全フェーズ分)
	int m_currentPhase = 0;//今のフェーズ
	std::vector<std::weak_ptr<EnemyBase>> m_phaseEnemies;//今のフェーズで出した敵//全員倒したら次のフェーズへ

	bool m_wasFPressed = false;//Fキーの押しっぱなし判定用
	bool m_wasIPressed = false;//Iキーの押しっぱなし判定用
	bool m_isDebugIdle = false;//デバッグ用//trueなら敵をずっとIdleにする
};
