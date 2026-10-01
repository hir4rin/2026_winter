#pragma once
#include "AnimData.h"
#include <vector>
#include <string>

/// <summary>
/// ゲーム全体のCSVデータの管理を行うクラス
/// 起動時にLoadAll()を呼び出して、全てのCSVデータを読み込むことを想定している
/// </summary>
class DataManager
{
public:
	static DataManager& GetInstance()
	{
		static DataManager instance;
		return instance;
	}
	/// <summary>
	/// 起動時に一度だけ呼び出す
	/// </summary>
	void LoadAll();

	/// <summary>
	/// ComboChain.csvを読み込み直す(デバッグ用)
	/// </summary>
	void ReloadComboRawData() { LoadComboRawData(); }

	const AnimData& GetPlayerAnimData() const { return m_playerAnimData;}
	const AnimData& GetEnemySwordmanAnimData() const { return m_enemySwordmanAnimData; }
	const AnimData& GetBossAnimData() const { return m_bossAnimData;}
	const std::vector<std::vector<std::string>>& GetComboRawData() const { return m_comboRawData;}
	//今後追加する場合はここにgetterを追加する
	const std::vector<std::vector<std::string>>& GetSpawnData() const { return m_spawnData; }//敵のスポーンデータを返す
private:
	//シングルトンパターンの実装
	DataManager() = default;
	~DataManager() = default;
	DataManager(const DataManager&) = delete;
	DataManager& operator=(const DataManager&) = delete;

	void LoadAnimData(const std::string& filePath,AnimData& animData);

	void LoadPlayerAnimData(); //プレイヤーのアニメーションデータを読み込む関数
	void LoadEnemySwordmanAnimData();
	void LoadBossAnimData(); //ボスのアニメーションデータを読み込む関数
	void LoadComboRawData(); //コンボの生データを読み込む関数
	//今後追加する場合はここにLoad関数を追加する
	void LoadSpawnData(); //敵のスポーンデータを読み込む関数

	AnimData m_playerAnimData;//プレイヤーのアニメーションデータ
	AnimData m_enemySwordmanAnimData;//敵(ソードマン)のアニメデータ
	AnimData m_bossAnimData;//ボスのアニメーションデータ
	std::vector<std::vector<std::string>> m_comboRawData;//コンボの生データ
	//今後追加する場合はここにデータのメンバ変数を追加する
	std::vector<std::vector<std::string>> m_spawnData;//敵のスポーンデータ

};

