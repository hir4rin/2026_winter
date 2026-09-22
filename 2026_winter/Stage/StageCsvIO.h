#pragma once
#include "StageObjectData.h"
#include <vector>
#include <string>

/// <summary>
/// ステージ番号ごとのBOX配置データ(CSV)の読み書きを行う
/// ステージを増やしたい場合は、stageNumberを変えて呼び出すだけでよい
/// (ファイルはdata/Stage/StageObjects/Stage001.csvのように自動で振り分けられる)
/// </summary>
class StageCsvIO
{
public:
	/// <summary>
	/// 指定したステージ番号のCSVを読み込む。ファイルが無ければ空のリストを返す
	/// </summary>
	static std::vector<StageObjectData> Load(int stageNumber);

	/// <summary>
	/// 指定したステージ番号のCSVに書き出す(既存ファイルは上書きする)
	/// </summary>
	/// <returns>書き込みに成功したらtrue</returns>
	static bool Save(int stageNumber, const std::vector<StageObjectData>& objects);

	/// <summary>
	/// ステージ番号からCSVファイルパスを組み立てる
	/// </summary>
	static std::string MakeFilePath(int stageNumber);

private:
	static constexpr const char* kDirectory = "data/Stage/StageObjects/";
	static constexpr const char* kHeaderComment = "#posX,posY,posZ,halfExtentX,halfExtentY,halfExtentZ";
};
