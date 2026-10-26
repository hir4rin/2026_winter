#pragma once
#include "Main/System.h"
#include <array>

/// <summary>
/// 遊べるステージの種類(StageSelectSceneで選んで、SceneMainに渡す)
/// ステージを増やす場合は、ここに追加してkStageInfosにも1行足す
/// </summary>
enum class StageType : int
{
	Test,//今までのテストステージ
	Demo,//NINJA GAIDEN 4 Chapter0〜2を参考にしたデモステージ(UnityのDemoStageBuilderで生成)
	Train,//NINJA GAIDEN 4 Chapter0 0-1 DDO列車(6両)の上で戦うステージ(tools/TrainStage で生成)
	Num
};

/// <summary>
/// ステージ1つ分の情報
/// </summary>
struct StageInfo
{
	const char* name;//ステージセレクトに出す名前
	const char* description;//ステージセレクトに出す説明
	AsyncData model;//ステージのモデル(System::LoadAllで読み込んだもの)
	AsyncData colModel;//当たり判定に使うモデル(見た目と同じでよければmodelと同じものを入れる)
	bool useTrainTunnel;//列車ステージのトンネルを流し続けるか
	int wallZoneNumber;//壁ゾーンのCSV番号(data/Stage/WallZones/Stage001.csv など)
	int enemySpawnNumber;//敵配置のCSV番号(data/Stage/Enemies/Enemies003.csv など)//ファイルが無ければ今まで通りテスト用の敵を1体出す
	                     //フェーズ進行(data/Stage/PhaseFlow/PhaseStage003.csv)とエリア(data/Stage/PhaseAreas/Stage003.csv)の番号も兼ねる(SceneMain::Init で読む)
};


//stageのデータをもらってそれに合うものを表示、保持
namespace StageData
{
	inline constexpr std::array<StageInfo, static_cast<size_t>(StageType::Num)> kStageInfos =
	{ {
		{ "テストステージ", "今までの動作確認用ステージ", AsyncData::TitleStageModel, AsyncData::TitleStageModel, false, 1, 1 },
		{ "デモステージ", "屋上 → 通り → 高架道路 → ボス広場(壁走り・壁キックで進む)", AsyncData::DemoStageModel, AsyncData::DemoStageModel, false, 3, 3 },
		{ "列車ステージ", "トンネルを走るDDO列車の上で戦う。平台車 → 車内 → スロープで屋根 → 車内 → 最後尾", AsyncData::TrainStageModel, AsyncData::TrainStageColModel, true, 4, 4 },
	} };

	inline const StageInfo& GetInfo(StageType type)
	{
		return kStageInfos[static_cast<size_t>(type)];
	}
}
