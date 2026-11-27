#pragma once
#include "Math/Vector3.h"
#include <memory>
#include <vector>

class EnemyManager;
class EnemyBase;
class Player;

/// <summary>
/// トレーニングルームの敵の出し方・動き・無限(設定メニューで変えた値で動かす)
/// SceneMain が持つ(トレーニングのステージのときだけ)
/// </summary>
class TrainingManager
{
public:
	//設定メニューの値(番号はメニューの選択肢の順)
	struct Settings
	{
		int enemyType = 0;//刀 / 銃 / バズーカ / 混ぜる / ボス(Kingmech)
		int enemyCount = 1;//1 / 3 / 5
		int enemyAi = 0;//立っているだけ / ガードし続ける / 攻撃してくる / ふつう
		int respawn = 0;//すぐ出し直す / 出し直さない
		int enemyHp = 0;//ふつう / 無限
		int partBreak = 0;//ふつう(確率) / 必ず / しない
		int playerHp = 0;//無限 / ふつう
		int gauge = 0;//無限 / ふつう
		bool operator==(const Settings&)const = default;
	};
	enum class EnemyTypeItem { Swordman, Gunner, Bazooka, Mix, Boss, Num };
	enum class EnemyCountItem { One, Three, Five, Num };

	void Init(std::weak_ptr<EnemyManager> enemyManager, std::weak_ptr<Player> player, const Settings& settings);
	void Update();

	const Settings& GetSettings()const { return m_settings; }
	//今の設定で敵を全部消して出し直す
	void Respawn();
	//プレイヤーをスタートに戻す
	void ReturnPlayerToStart();

private:
	//敵1体分の置き場所
	struct Slot
	{
		int type = 0;//EnemyType
		Vector3 pos;
		std::weak_ptr<EnemyBase> enemy;
		int respawnTimer = -1;//倒れてからのフレーム(-1なら生きている)
	};
	void SpawnSlot(Slot& slot);
	void ApplyAi(EnemyBase& enemy)const;

	std::weak_ptr<EnemyManager> m_enemyManager;
	std::weak_ptr<Player> m_player;
	Settings m_settings;
	std::vector<Slot> m_slots;
};
