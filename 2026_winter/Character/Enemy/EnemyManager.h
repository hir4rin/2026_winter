#pragma once
#include <memory>
#include <vector>
#include <deque>

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
private:
	void SpawnEnemy();
	void UpdateCorpses();//死体の数が上限を超えたら、古い順に消す
private:
	static constexpr size_t kMaxCorpseNum = 5;//残しておく死体の最大数

	std::vector<std::shared_ptr<EnemyBase>> m_enemies;
	std::deque<std::shared_ptr<EnemyBase>> m_corpses;//死体を死んだ順に並べたもの

	std::weak_ptr<Player> m_player;//プレイヤーの弱参照

	bool m_wasFPressed = false;//Fキーの押しっぱなし判定用
};

