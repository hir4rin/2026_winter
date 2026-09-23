#include "DataManager.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <string>

namespace
{
    const  std::string kPlayerAnimPath = "data/Player_CSV/PlayerAnim.csv";
    const std::string kEnemySwordmanAnimPath = "data/Enemy/EnemySwordmanAnim.csv";

    const std::string kBossAnimPath = "data/Enemy/Boss/BossAnim.csv";

}

void DataManager::LoadAll()
{
	//LoadPlayerAnimData();
	LoadBossAnimData();

    LoadAnimData(kPlayerAnimPath, m_playerAnimData);
    LoadAnimData(kEnemySwordmanAnimPath, m_enemySwordmanAnimData);


    LoadComboRawData();
    LoadSpawnData();
	//今後追加する場合はここにLoad関数を呼び出すコード
}

void DataManager::LoadAnimData(const std::string& filePath, AnimData& animData)
{
    std::ifstream file(filePath);
    assert(file.is_open() && "Animationが開けませんでした");

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') continue;

        auto comma = line.find(',');
        if (comma == std::string::npos) continue;

        std::string key = line.substr(0, comma);
        std::string value = line.substr(comma + 1);
        animData.animNames[key] = value;
    }
}

void DataManager::LoadEnemySwordmanAnimData()
{
}

void DataManager::LoadBossAnimData()
{
   /* std::ifstream file("data/Enemy/Boss/BossAnim.csv");
    assert(file.is_open() && "BossAnim.csvが開けませんでした");

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#') continue;

        auto comma = line.find(',');
        if (comma == std::string::npos) continue;

        std::string key = line.substr(0, comma);
        std::string value = line.substr(comma + 1);
        m_bossAnimData.animNames[key] = value;
    }*/
}

void DataManager::LoadComboRawData()
{
	std::ifstream file("data/Player_CSV/ComboChain.csv");
	assert(file.is_open() && "ComboChain.csvが開けませんでした");

    std::string line;
    while (std::getline(file, line))
    {
		if (line.empty() || line[0] == '#') continue;

		std::istringstream ss(line);
        std::string token;
		std::vector<std::string> tokens;
		while (std::getline(ss, token, ','))//カンマ区切りでトークンを取得//分割
        {
			tokens.push_back(token);
        }
		m_comboRawData.push_back(tokens);//分割したトークンのベクターを生データとして保存する
    }
}

void DataManager::LoadSpawnData()
{
    //std::ifstream file("data/SpawnData/SpawnData.csv");
    //assert(file.is_open() && "SpawnData.csvが開けませんでした");

    //std::string line;
    //while (std::getline(file, line))
    //{
    //    if (line.empty() || line[0] == '#') continue;

    //    std::istringstream ss(line);
    //    std::string token;
    //    std::vector<std::string> tokens;
    //    while (std::getline(ss, token, ','))//カンマ区切りでトークンを取得//分割
    //    {
    //        tokens.push_back(token);
    //    }
    //    m_spawnData.push_back(tokens);//分割したトークンのベクターを生データとして保存する
    //}
}
