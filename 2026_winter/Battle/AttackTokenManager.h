#pragma once
#include <memory>
#include <vector>

class EnemyBase;

//敵の攻撃権の管理//BattleManagerが持つ
class AttackTokenManager
{
public:
	struct Holder
	{
		std::weak_ptr<EnemyBase> holder;
		int cost = 1;
	};

	AttackTokenManager() = default;
	~AttackTokenManager() = default;
	//コピー禁止
	AttackTokenManager(const AttackTokenManager&) = delete;
	AttackTokenManager& operator=(const AttackTokenManager&) = delete;

	/// <summary>
	/// 攻撃権を渡す
	/// </summary>
	/// <param name="enemy"></param>
	/// <param name="cost">攻撃の重さ</param>
	/// <returns>取れたらtrueで、既に持っていてもtrue</returns>
	bool Request(EnemyBase& enemy, int cost = 1);

	//攻撃権を返す//持っていなければ何もしない
	void Release(const EnemyBase& enemy);



	//攻撃権を持っているか
	bool HasToken(const EnemyBase& enemy)const;

	//今攻撃権が取れる状態かどうか
	bool CanRequest(int cost = 1)const;

	//攻撃権をクリア
	void AllClear();

	//攻撃間隔を減らす・死んだ敵の攻撃権を回収
	void Update();

	int GetUsedCost()const;//使われている重さの合計
	int GetMaxCost()const { return m_maxCost; }
	float GetGlobalInterval()const { return m_globalInterval; }


private:
	std::vector<Holder> m_holders;

	int m_maxCost = 2;//同時に使える重さの合計
	float m_globalInterval = 0.0f;//次に攻撃権を渡せるまでの時間

};

