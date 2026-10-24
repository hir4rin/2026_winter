#pragma once
#include "AnimData.h"
#include "Stage/WallZone/WallZoneData.h"
#include "Stage/Phase/EnemySpawnData.h"
#include "Stage/Phase/PhaseFlowData.h"
#include "Stage/Phase/PhaseAreaData.h"
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
	/// <summary>
	/// EnemySwordmanAttack.csvを読み込み直す(デバッグ用)
	/// </summary>
	void ReloadEnemySwordmanAttackRawData() { LoadEnemySwordmanAttackRawData(); }
	/// <summary>
	/// EnemyGunnerAttack.csvを読み込み直す(デバッグ用)
	/// </summary>
	void ReloadEnemyGunnerAttackRawData() { LoadEnemyGunnerAttackRawData(); }
	/// <summary>
	/// EnemyBazookaAttack.csvを読み込み直す(デバッグ用)
	/// </summary>
	void ReloadEnemyBazookaAttackRawData() { LoadEnemyBazookaAttackRawData(); }
	/// <summary>
	/// BossEnemyPirateAttack.csvを読み込み直す(デバッグ用)
	/// </summary>
	void ReloadBossEnemyPirateAttackRawData() { LoadBossEnemyPirateAttackRawData(); }
	/// <summary>
	/// BossEnemyPirateAnim.csvを読み込み直す(デバッグ用)
	/// </summary>
	void ReloadBossEnemyPirateAnimData();
	/// <summary>
	/// EnemyStatus.csvを読み込み直す(デバッグ用)
	/// </summary>
	void ReloadEnemyStatusRawData() { LoadEnemyStatusRawData(); }

	const AnimData& GetPlayerAnimData() const { return m_playerAnimData;}
	const AnimData& GetEnemySwordmanAnimData() const { return m_enemySwordmanAnimData; }
	const std::vector<std::vector<std::string>>& GetEnemySwordmanAttackRawData() const { return m_enemySwordmanAttackRawData; }//敵(ソードマン)の攻撃の生データ//解釈はEnemyBase::InitializeAttackList
	const AnimData& GetEnemyGunnerAnimData() const { return m_enemyGunnerAnimData; }
	const std::vector<std::vector<std::string>>& GetEnemyGunnerAttackRawData() const { return m_enemyGunnerAttackRawData; }//敵(ガンナー)の攻撃の生データ
	const AnimData& GetEnemyBazookaAnimData() const { return m_enemyBazookaAnimData; }
	const std::vector<std::vector<std::string>>& GetEnemyBazookaAttackRawData() const { return m_enemyBazookaAttackRawData; }//敵(バズーカ兵)の攻撃の生データ
	const AnimData& GetBossEnemyPirateAnimData() const { return m_bossEnemyPirateAnimData; }
	const std::vector<std::vector<std::string>>& GetBossEnemyPirateAttackRawData() const { return m_bossEnemyPirateAttackRawData; }//ボス(海賊)の攻撃の生データ
	const AnimData& GetBossArmAnimData() const { return m_bossArmAnimData; }//ボスの背中の腕のアニメデータ
	const std::vector<std::vector<std::string>>& GetEnemyStatusRawData() const { return m_enemyStatusRawData; }//敵のステータスの生データ(1行=1種類)//解釈はEnemyBase::InitializeStatus
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
	/// 指定したステージ番号の敵配置CSV(data/Stage/Enemies/Enemies003.csvなど)を読み込む
	/// ファイルが無ければ空のデータになる
	/// </summary>
	void LoadEnemySpawnData(int stageNumber);
	/// <summary>最後にLoadした敵配置のデータを返す</summary>
	const std::vector<EnemySpawnData>& GetEnemySpawnData() const { return m_enemySpawnData; }
	/// <summary>ステージ番号から敵配置CSVのファイルパスを組み立てる</summary>
	static std::string GetEnemySpawnFilePath(int stageNumber);
	//---------------------------------------------------------------------------

	//フェーズの遷移表-----------------------------------------------------------
	void LoadPhaseFlow(int stageNumber);//無ければ空
	const std::vector<PhaseTransition>& GetPhaseFlow() const { return m_phaseFlow; }
	static std::string GetPhaseFlowFilePath(int stageNumber);
	//---------------------------------------------------------------------------

	//フェーズ進行のエリア-------------------------------------------------------
	void LoadPhaseAreas(int stageNumber);//無ければ空
	bool SavePhaseAreas(int stageNumber, const std::vector<PhaseArea>& areas);//エディタから保存
	const std::vector<PhaseArea>& GetPhaseAreas() const { return m_phaseAreas; }
	static std::string GetPhaseAreaFilePath(int stageNumber);
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
	void LoadEnemySwordmanAttackRawData(); //敵(ソードマン)の攻撃の生データを読み込む関数
	void LoadEnemyGunnerAttackRawData(); //敵(ガンナー)の攻撃の生データを読み込む関数
	void LoadEnemyBazookaAttackRawData(); //敵(バズーカ兵)の攻撃の生データを読み込む関数
	void LoadBossEnemyPirateAttackRawData(); //ボス(海賊)の攻撃の生データを読み込む関数
	void LoadRawCsv(const std::string& filePath, std::vector<std::vector<std::string>>& rawData);//#の行を飛ばして1行ずつ分割する
	void LoadEnemyStatusRawData(); //敵のステータスの生データを読み込む関数
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
	//遷移表CSVの文字列とenumの変換
	static bool StringToPhaseCondition(const std::string& str, PhaseConditionType& outCondition);
	static bool StringToPhaseEvent(const std::string& str, PhaseEvent& outEvent);
	/// <summary>strをseparatorで分割する(空の要素も残す)</summary>
	static std::vector<std::string> Split(const std::string& str, char separator);
	/// <summary>
	/// route列("x:y:z:wait|x:y:z:wait|...")を巡回ポイントの配列にする
	/// 数値にできない値があったら例外を投げる(呼び出し側で行ごとスキップする)//Parseは変換という意味
	/// </summary>
	static std::vector<EnemySpawnRoutePoint> ParseRoute(const std::string& str);
	/// <summary>名前にカンマや改行が入るとCSVが壊れるので、別の文字に置き換える</summary>//Sanitizeは消毒、きれいにする(変換、取り除いたりすｒ
	static std::string SanitizeCsvCell(const std::string& str);
	//---------------------------------------------------------------------------

	AnimData m_playerAnimData;//プレイヤーのアニメーションデータ
	AnimData m_enemySwordmanAnimData;//敵(ソードマン)のアニメデータ
	std::vector<std::vector<std::string>> m_enemySwordmanAttackRawData;//敵(ソードマン)の攻撃の生データ
	AnimData m_enemyGunnerAnimData;//敵(ガンナー)のアニメデータ
	std::vector<std::vector<std::string>> m_enemyGunnerAttackRawData;//敵(ガンナー)の攻撃の生データ
	AnimData m_enemyBazookaAnimData;//敵(バズーカ兵)のアニメデータ
	std::vector<std::vector<std::string>> m_enemyBazookaAttackRawData;//敵(バズーカ兵)の攻撃の生データ
	AnimData m_bossEnemyPirateAnimData;//ボス(海賊)のアニメデータ
	std::vector<std::vector<std::string>> m_bossEnemyPirateAttackRawData;//ボス(海賊)の攻撃の生データ
	AnimData m_bossArmAnimData;//ボスの背中の腕のアニメデータ
	std::vector<std::vector<std::string>> m_enemyStatusRawData;//敵のステータスの生データ
	AnimData m_bossAnimData;//ボスのアニメーションデータ
	std::vector<std::vector<std::string>> m_comboRawData;//コンボの生データ
	//今後追加する場合はここにデータのメンバ変数を追加する
	std::vector<std::vector<std::string>> m_spawnData;//敵のスポーンデータ
	std::vector<std::vector<std::vector<std::string>>> m_patrolRouteRawData;//巡回ルートの生データ//[ルート番号][行][列]
	std::vector<WallZoneData> m_wallZoneData;//壁ゾーンのデータ(最後にLoad/Saveしたステージのもの)
	std::vector<EnemySpawnData> m_enemySpawnData;//敵配置のデータ(最後にLoadしたステージのもの)
	std::vector<PhaseTransition> m_phaseFlow;//遷移表
	std::vector<PhaseArea> m_phaseAreas;//フェーズ進行のエリア

};

