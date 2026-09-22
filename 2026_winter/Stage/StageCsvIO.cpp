#include "StageCsvIO.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

std::string StageCsvIO::MakeFilePath(int stageNumber)
{
	std::ostringstream oss;
	oss << kDirectory << "Stage" << std::setw(3) << std::setfill('0') << stageNumber << ".csv";
	return oss.str();
}

std::vector<StageObjectData> StageCsvIO::Load(int stageNumber)
{
	std::vector<StageObjectData> objects;

	std::ifstream file(MakeFilePath(stageNumber));
	if (!file.is_open())
	{
		//ファイルが無い場合は空のステージとして扱う(新規ステージ作成時など)
		return objects;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#')continue;//空行・コメント行はスキップ

		std::istringstream lineStream(line);
		std::string cell;
		float values[6] = {};
		int i = 0;
		while (std::getline(lineStream, cell, ',') && i < 6)
		{
			values[i] = std::stof(cell);
			++i;
		}
		if (i < 6)continue;//列数が足りない不正な行はスキップ

		StageObjectData data;
		data.position = Vector3(values[0], values[1], values[2]);
		data.halfExtents = Vector3(values[3], values[4], values[5]);
		objects.push_back(data);
	}

	return objects;
}

bool StageCsvIO::Save(int stageNumber, const std::vector<StageObjectData>& objects)
{
	//出力先フォルダが無ければ作る
	std::filesystem::create_directories(kDirectory);

	std::ofstream file(MakeFilePath(stageNumber));
	if (!file.is_open())
	{
		return false;
	}

	file << kHeaderComment << "\n";
	for (const auto& obj : objects)
	{
		file << obj.position.x << "," << obj.position.y << "," << obj.position.z << ","
			<< obj.halfExtents.x << "," << obj.halfExtents.y << "," << obj.halfExtents.z << "\n";
	}

	return true;
}
