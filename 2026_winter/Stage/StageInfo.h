#pragma once
#include "../System.h"
#include <array>

/// <summary>
/// 遊べるステージの種類(StageSelectSceneで選んで、SceneMainに渡す)
/// ステージを増やす場合は、ここに追加してkStageInfosにも1行足す
/// </summary>
enum class StageType : int
{
	Test,//今までのテストステージ
	Demo,//NINJA GAIDEN 4 Chapter0〜2を参考にしたデモステージ(UnityのDemoStageBuilderで生成)
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
	int wallZoneNumber;//壁ゾーンのCSV番号(data/Stage/WallZones/Stage001.csv など)
	int enemySpawnNumber;//敵配置のCSV番号(data/Stage/Enemies/Stage003.csv など)//ファイルが無ければ今まで通りテスト用の敵を1体出す
};

namespace StageData
{
	inline constexpr std::array<StageInfo, static_cast<size_t>(StageType::Num)> kStageInfos =
	{ {
		{ "テストステージ", "今までの動作確認用ステージ", AsyncData::TitleStageModel, 1, 1 },
		{ "デモステージ", "屋上 → 通り → 高架道路 → ボス広場(壁走り・壁キックで進む)", AsyncData::DemoStageModel, 3, 3 },
	} };

	inline const StageInfo& GetInfo(StageType type)
	{
		return kStageInfos[static_cast<size_t>(type)];
	}
}
