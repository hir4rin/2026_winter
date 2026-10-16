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
	//デバッグ描画//文字を出すので、3D・UIを描き終わった後に呼ぶこと//F3で表示を切り替える
	void DebugDraw();


	void SetPlayer(std::weak_ptr<Player> player) { m_player = player; }

	const std::vector<std::shared_ptr<EnemyBase>>& GetEnemies()const { return m_enemies; }

	//フェーズ-----------------------------------------------------------------
	void SetSpawnData(const std::vector<EnemySpawnData>& spawnData) { m_spawnData = spawnData; }
	void StartPhase(int phase);//そのフェーズの敵を出す
	bool IsPhaseEnemiesAllDead()const;
	int GetTotalKillCount()const { return m_totalKillCount; }
	//Group内の敵がすべて死んだかどうか
	bool IsGroupAllDead(const std::string& groupId)const;
private:
	void SpawnEnemy();
	void UpdateCorpses();//死体の数が上限を超えたら、古い順に消す

	std::shared_ptr<EnemyBase> CreateEnemy(const EnemySpawnData& data);//配置データ1つ分の敵を作る

	//グループ-----------------------------------------------------------------
	void UpdateGroupAlert();//同じグループの誰かがプレイヤーを見つけていたら、全員に気づかせる
	
private:
	static constexpr size_t kMaxCorpseNum = 5;//残しておく死体の最大数

	std::vector<std::shared_ptr<EnemyBase>> m_enemies;
	std::deque<std::shared_ptr<EnemyBase>> m_corpses;//死体を死んだ順に並べたもの

	std::weak_ptr<Player> m_player;//プレイヤーの弱参照

	std::vector<EnemySpawnData> m_spawnData;//敵配置のデータ(全フェーズ分)
	std::vector<std::weak_ptr<EnemyBase>> m_phaseEnemies;//今のフェーズで出した敵
	int m_totalKillCount = 0;//倒した数の合計

	bool m_wasFPressed = false;//Fキーの押しっぱなし判定用
	bool m_wasIPressed = false;//Iキーの押しっぱなし判定用
	bool m_isDebugIdle = false;//デバッグ用//trueなら敵をずっとIdleにする
	bool m_wasF3Pressed = false;//F3キーの押しっぱなし判定用
	bool m_isDebugDraw = true;//デバッグ用//trueなら敵のデバッグ表示を出す
};
