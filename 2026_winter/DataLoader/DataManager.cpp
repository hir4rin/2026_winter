#include "DataManager.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <string>
#include <iomanip>
#include <filesystem>

namespace
{
    //const  std::string kPlayerAnimPath = "data/Player_CSV/PlayerAnim.csv";
    const  std::string kPlayerAnimPath = "data/Player_CSV/Player01Anim.csv";

	//const std::string kPlayerComboChainPath = "data/Player_CSV/ComboChain.csv";
	const std::string kPlayerComboChainPath = "data/Player_CSV/ComboChain01.csv";

    const std::string kEnemySwordmanAnimPath = "data/Enemy/EnemySwordmanAnim.csv";

    const std::string kBossAnimPath = "data/Enemy/Boss/BossAnim.csv";

    const std::string kPatrolRoutePath = "data/Enemy/Patrol/PatrolRoute";//後ろにルート番号と".csv"をつける

    //壁ゾーン
    const std::string kWallZoneDirectory = "data/Stage/WallZones/";//後ろに"Stage001.csv"のようにステージ番号をつける
    const char* const kWallZoneHeaderComment = "#type,name,posX,posY,posZ,halfExtentX,halfExtentY,halfExtentZ,rotYDeg,isActive";
    constexpr int kWallZoneColumnNum = 10;//壁ゾーンCSVの1行の列数

    //敵の配置
    const std::string kEnemySpawnDirectory = "data/Stage/Enemies/";//後ろに"Stage003.csv"のようにステージ番号をつける
    //敵配置CSVの列番号
    //#type,posX,posY,posZ,rotYDeg,phase,initialState,groupId,route
    enum EnemySpawnColumn : int
    {
        Type = 0,
        PosX,
        PosY,
        PosZ,
        RotYDeg,
        Phase,
        InitialState,
        GroupId,
        Route,//"x:y:z:wait|x:y:z:wait|..."//空なら巡回しない
        Size,//列数
    };
    constexpr int kEnemySpawnMinColumnNum = EnemySpawnColumn::Route;//route列が空だと末尾の列が無くなるので、route以外があればOKにする
    constexpr char kRoutePointSeparator = '|';//巡回ポイント同士の区切り
    constexpr char kRouteValueSeparator = ':';//巡回ポイントの中の値(x,y,z,wait)の区切り
    constexpr int kRouteValueNum = 4;//巡回ポイント1つの値の数(x,y,z,wait)
}

void DataManager::LoadAll()
{
	//LoadPlayerAnimData();
	LoadBossAnimData();

    LoadAnimData(kPlayerAnimPath, m_playerAnimData);
    LoadAnimData(kEnemySwordmanAnimPath, m_enemySwordmanAnimData);


    LoadComboRawData();
    LoadSpawnData();
    LoadPatrolRouteRawData();
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
	std::ifstream file(kPlayerComboChainPath);
	assert(file.is_open() && "ComboChain.csvが開けませんでした");

	m_comboRawData.clear();//読み込み直しのときに前のデータが残らないように消す

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

void DataManager::LoadPatrolRouteRawData()
{
    m_patrolRouteRawData.clear();//読み込み直しのときに前のデータが残らないように消す

    //PatrolRoute0.csv, PatrolRoute1.csv...と、ファイルが見つからなくなるまで順番に読み込む
    for (int routeId = 0; ; ++routeId)
    {
        std::ifstream file(kPatrolRoutePath + std::to_string(routeId) + ".csv");
        if (!file.is_open()) break;

        std::vector<std::vector<std::string>> routeData;
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
            routeData.push_back(tokens);//分割したトークンのベクターを生データとして保存する
        }
        m_patrolRouteRawData.push_back(routeData);//ルート番号順に保存する
    }
    assert(!m_patrolRouteRawData.empty() && "PatrolRoute0.csvが開けませんでした");
}

std::string DataManager::GetWallZoneFilePath(int stageNumber)
{
    std::ostringstream oss;
    oss << kWallZoneDirectory << "Stage" << std::setw(3) << std::setfill('0') << stageNumber << ".csv";
    return oss.str();
}

void DataManager::LoadWallZoneData(int stageNumber)
{
    m_wallZoneData.clear();//読み込み直しのときに前のデータが残らないように消す

    std::ifstream file(GetWallZoneFilePath(stageNumber));
    if (!file.is_open()) return;//ファイルが無い場合はゾーン無しとして扱う(新規ステージ作成時など)

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ','))//カンマ区切りでトークンを取得//分割
        {
            tokens.push_back(token);
        }
        if (tokens.size() < kWallZoneColumnNum) continue;//列数が足りない不正な行はスキップ

        WallZoneData data;
        if (!StringToWallZoneType(tokens[0], data.type)) continue;//知らないtypeはスキップ
        data.name = tokens[1];
        try
        {
            data.position = Vector3(std::stof(tokens[2]), std::stof(tokens[3]), std::stof(tokens[4]));
            data.halfExtents = Vector3(std::stof(tokens[5]), std::stof(tokens[6]), std::stof(tokens[7]));
            data.rotYDeg = std::stof(tokens[8]);
            data.isActive = std::stoi(tokens[9]) != 0;
        }
        catch (...)
        {
            continue;//数値にできない行はスキップ
        }
        m_wallZoneData.push_back(data);
    }
}

bool DataManager::SaveWallZoneData(int stageNumber, const std::vector<WallZoneData>& zones)
{
    //出力先フォルダが無ければ作る
    std::filesystem::create_directories(kWallZoneDirectory);

    std::ofstream file(GetWallZoneFilePath(stageNumber));
    if (!file.is_open()) return false;

    file << kWallZoneHeaderComment << "\n";
    for (const auto& zone : zones)
    {
        file << WallZoneTypeToString(zone.type) << "," << SanitizeCsvCell(zone.name) << ","
            << zone.position.x << "," << zone.position.y << "," << zone.position.z << ","
            << zone.halfExtents.x << "," << zone.halfExtents.y << "," << zone.halfExtents.z << ","
            << zone.rotYDeg << "," << (zone.isActive ? 1 : 0) << "\n";
    }
    if (!file.good()) return false;

    m_wallZoneData = zones;//保存した内容を持っているデータにも反映する
    return true;
}

std::string DataManager::GetEnemySpawnFilePath(int stageNumber)
{
    std::ostringstream oss;
    oss << kEnemySpawnDirectory << "Stage" << std::setw(3) << std::setfill('0') << stageNumber << ".csv";
    return oss.str();
}

void DataManager::LoadEnemySpawnData(int stageNumber)
{
    m_enemySpawnData.clear();//読み込み直しのときに前のデータが残らないように消す

    std::ifstream file(GetEnemySpawnFilePath(stageNumber));
    if (!file.is_open()) return;//ファイルが無い場合は敵無しとして扱う

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        auto tokens = Split(line, ',');
        if (tokens.size() < kEnemySpawnMinColumnNum) continue;//列数が足りない不正な行はスキップ

        EnemySpawnData data;
        if (!StringToEnemyType(tokens[EnemySpawnColumn::Type], data.type)) continue;//知らないtypeはスキップ
        if (!StringToEnemyInitialState(tokens[EnemySpawnColumn::InitialState], data.initialState)) continue;
        data.groupId = tokens[EnemySpawnColumn::GroupId];
        try
        {
            data.position = Vector3(std::stof(tokens[EnemySpawnColumn::PosX]),
                std::stof(tokens[EnemySpawnColumn::PosY]),
                std::stof(tokens[EnemySpawnColumn::PosZ]));
            data.rotYDeg = std::stof(tokens[EnemySpawnColumn::RotYDeg]);
            data.phase = std::stoi(tokens[EnemySpawnColumn::Phase]);
            if (tokens.size() > EnemySpawnColumn::Route)
            {
                data.route = ParseRoute(tokens[EnemySpawnColumn::Route]);
            }
        }
        catch (...)
        {
            continue;//数値にできない行はスキップ
        }
        m_enemySpawnData.push_back(data);
    }
}

const char* DataManager::WallZoneTypeToString(Collider::ColRole type)
{
    switch (type)
    {
    case Collider::ColRole::WallKickZone: return "WallKickZone";
    case Collider::ColRole::WallRunZone: return "WallRunZone";
    default: return "Unknown";
    }
}

bool DataManager::StringToWallZoneType(const std::string& str, Collider::ColRole& outType)
{
    if (str == "WallKickZone") { outType = Collider::ColRole::WallKickZone; return true; }
    if (str == "WallRunZone") { outType = Collider::ColRole::WallRunZone; return true; }
    return false;
}

bool DataManager::StringToEnemyType(const std::string& str, EnemyType& outType)
{
    if (str == "Swordman") { outType = EnemyType::Swordman; return true; }
    return false;
}

bool DataManager::StringToEnemyInitialState(const std::string& str, EnemyInitialState& outState)
{
    if (str == "Idle") { outState = EnemyInitialState::Idle; return true; }
    if (str == "Patrol") { outState = EnemyInitialState::Patrol; return true; }
    if (str == "Guard") { outState = EnemyInitialState::Guard; return true; }
    return false;
}

std::vector<std::string> DataManager::Split(const std::string& str, char separator)
{
    std::vector<std::string> result;
    std::istringstream ss(str);
    std::string token;
    while (std::getline(ss, token, separator))
    {
        result.push_back(token);
    }
    return result;
}

std::vector<EnemySpawnRoutePoint> DataManager::ParseRoute(const std::string& str)
{
    std::vector<EnemySpawnRoutePoint> route;
    for (const auto& pointStr : Split(str, kRoutePointSeparator))
    {
        if (pointStr.empty()) continue;
        auto values = Split(pointStr, kRouteValueSeparator);
        if (values.size() < kRouteValueNum) continue;//値が足りないポイントは無視する

        EnemySpawnRoutePoint point;
        point.pos = Vector3(std::stof(values[0]), std::stof(values[1]), std::stof(values[2]));
        point.waitTime = std::stof(values[3]);
        route.push_back(point);
    }
    return route;
}

std::string DataManager::SanitizeCsvCell(const std::string& str)
{
    std::string result = str;
    for (auto& c : result)
    {
        if (c == ',' || c == '\n' || c == '\r') c = '_';
    }
    return result;
}
