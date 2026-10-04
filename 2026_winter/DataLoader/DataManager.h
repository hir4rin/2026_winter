#pragma once
#include "AnimData.h"
#include "../Stage/WallZoneData.h"
#include "../Stage/EnemySpawnData.h"
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
	const std::vector<std::vector<std::vector<std::string>>>& GetPatrolRouteRawData() const { return m_patrolRouteRawData; }//巡回ルートの生データを返す//[ルート番号][行][列]

	//壁ゾーン(WallKickZone/WallRunZone)------------------------------------------
	/// <summary>
	/// 指定したステージ番号の壁ゾーンCSV(data/Stage/WallZones/Stage001.csvなど)を読み込む
	/// ファイルが無ければ空のデータになる(新規ステージ作成時など)
	/// </summary>
	void LoadWallZoneData(int stageNumber);
	/// <summary>指定したステージ番号の壁ゾーンCSVに書き出す(既存ファイルは上書きする)。成功したら持っているデータも更新する</summary>
	/// <returns>書き込みに成功したらtrue</returns>
	bool SaveWallZoneData(int stageNumber, const std::vector<WallZoneData>& zones);
	/// <summary>最後にLoad/Saveした壁ゾーンのデータを返す</summary>
	const std::vector<WallZoneData>& GetWallZoneData() const { return m_wallZoneData; }
	/// <summary>ステージ番号から壁ゾーンCSVのファイルパスを組み立てる</summary>
	static std::string GetWallZoneFilePath(int stageNumber);
	//---------------------------------------------------------------------------

	//敵の配置(UnityのEnemySpawnExporterで書き出したもの)--------------------------
	/// <summary>
	/// 指定したステージ番号の敵配置CSV(data/Stage/Enemies/Stage003.csvなど)を読み込む
	/// ファイルが無ければ空のデータになる
	/// </summary>
	void LoadEnemySpawnData(int stageNumber);
	/// <summary>最後にLoadした敵配置のデータを返す</summary>
	const std::vector<EnemySpawnData>& GetEnemySpawnData() const { return m_enemySpawnData; }
	/// <summary>ステージ番号から敵配置CSVのファイルパスを組み立てる</summary>
	static std::string GetEnemySpawnFilePath(int stageNumber);
	//---------------------------------------------------------------------------
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
	void LoadPatrolRouteRawData(); //巡回ルートの生データを読み込む関数

	//CSV読み書き用の補助関数-----------------------------------------------------
	//壁ゾーンのtypeとCSVに書く文字列の変換
	static const char* WallZoneTypeToString(Collider::ColRole type);
	static bool StringToWallZoneType(const std::string& str, Collider::ColRole& outType);
	//敵配置CSVの文字列とenumの変換
	static bool StringToEnemyType(const std::string& str, EnemyType& outType);
	static bool StringToEnemyInitialState(const std::string& str, EnemyInitialState& outState);
	/// <summary>strをseparatorで分割する(空の要素も残す)</summary>
	static std::vector<std::string> Split(const std::string& str, char separator);
	/// <summary>
	/// route列("x:y:z:wait|x:y:z:wait|...")を巡回ポイントの配列にする
	/// 数値にできない値があったら例外を投げる(呼び出し側で行ごとスキップする)
	/// </summary>
	static std::vector<EnemySpawnRoutePoint> ParseRoute(const std::string& str);
	/// <summary>名前にカンマや改行が入るとCSVが壊れるので、別の文字に置き換える</summary>
	static std::string SanitizeCsvCell(const std::string& str);
	//---------------------------------------------------------------------------

	AnimData m_playerAnimData;//プレイヤーのアニメーションデータ
	AnimData m_enemySwordmanAnimData;//敵(ソードマン)のアニメデータ
	AnimData m_bossAnimData;//ボスのアニメーションデータ
	std::vector<std::vector<std::string>> m_comboRawData;//コンボの生データ
	//今後追加する場合はここにデータのメンバ変数を追加する
	std::vector<std::vector<std::string>> m_spawnData;//敵のスポーンデータ
	std::vector<std::vector<std::vector<std::string>>> m_patrolRouteRawData;//巡回ルートの生データ//[ルート番号][行][列]
	std::vector<WallZoneData> m_wallZoneData;//壁ゾーンのデータ(最後にLoad/Saveしたステージのもの)
	std::vector<EnemySpawnData> m_enemySpawnData;//敵配置のデータ(最後にLoadしたステージのもの)

};

