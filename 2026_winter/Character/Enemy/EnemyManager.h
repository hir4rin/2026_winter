#pragma once
#include <memory>
#include <vector>

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
	std::vector<std::shared_ptr<EnemyBase>> m_enemies;

	std::weak_ptr<Player> m_player;//プレイヤーの弱参照
};

