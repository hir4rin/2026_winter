#pragma once
#include "../Math/Vector3.h"
#include <string>
#include <vector>

/// <summary>
/// 敵の種類(CSVのtype列の文字列と対応させる)
/// 種類を増やす場合は、こことDataManager.cppのStringToEnemyType、EnemyManager::CreateEnemyに追加する
/// </summary>
enum class EnemyType : int
{
	Swordman,
};

/// <summary>
/// 出現したときの最初の状態
/// </summary>
enum class EnemyInitialState : int
{
	Idle,//すぐに戦闘を始める(今までの敵と同じ)
	Patrol,//巡回ルートを回りながらプレイヤーを探す(ルートが無ければGuardになる)
	Guard,//その場に立ったまま、向いている方向でプレイヤーを探す
};

/// <summary>
/// 巡回ポイント1つ分
/// </summary>
struct EnemySpawnRoutePoint
{
	Vector3 pos;
	float waitTime = 0.0f;//着いてから次へ出発するまでの待機フレーム数//負の値ならずっと待機
};

/// <summary>
/// 敵1体分の配置データ
/// UnityのEnemySpawnExporterで書き出したCSV(data/Stage/Enemies/Stage003.csvなど)の1行に対応する
/// </summary>
struct EnemySpawnData
{
	EnemyType type = EnemyType::Swordman;
	Vector3 position;//ワールド座標での足元の位置
	float rotYDeg = 0.0f;//Y軸回転(度)//0で+Zを向く(Unityと同じ)
	int phase = 1;//このフェーズが始まったら出現する
	EnemyInitialState initialState = EnemyInitialState::Idle;
	std::string groupId;//空でなければ、同じグループの1体がプレイヤーを見つけると全員が気づく
	std::vector<EnemySpawnRoutePoint> route;//巡回ルート//空なら巡回しない
};
