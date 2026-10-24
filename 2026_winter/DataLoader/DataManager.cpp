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
    const std::string kEnemySwordmanAttackPath = "data/Enemy/EnemySwordmanAttack.csv";
    const std::string kEnemyGunnerAnimPath = "data/Enemy/EnemyGunnerAnim.csv";
    const std::string kEnemyGunnerAttackPath = "data/Enemy/EnemyGunnerAttack.csv";
    const std::string kEnemyBazookaAnimPath = "data/Enemy/EnemyBazookaAnim.csv";
    const std::string kEnemyBazookaAttackPath = "data/Enemy/EnemyBazookaAttack.csv";
    const std::string kBossEnemyPirateAnimPath = "data/Enemy/BossEnemyPirateAnim.csv";
    const std::string kBossEnemyPirateAttackPath = "data/Enemy/BossEnemyPirateAttack.csv";
    const std::string kBossArmAnimPath = "data/Enemy/BossArmAnim.csv";//ボスの背中の腕//アニメ名はボスのモデルのもの
    const std::string kEnemyStatusPath = "data/Enemy/EnemyStatus.csv";

    const std::string kBossAnimPath = "data/Enemy/Boss/BossAnim.csv";

    const std::string kPatrolRoutePath = "data/Enemy/Patrol/PatrolRoute";//後ろにルート番号と".csv"をつける

    //壁ゾーン
    const std::string kWallZoneDirectory = "data/Stage/WallZones/";//後ろに"Stage001.csv"のようにステージ番号をつける
    const char* const kWallZoneHeaderComment = "#type,name,posX,posY,posZ,halfExtentX,halfExtentY,halfExtentZ,rotYDeg,isActive";
    constexpr int kWallZoneColumnNum = 10;//壁ゾーンCSVの1行の列数

    //敵の配置
    const std::string kEnemySpawnDirectory = "data/Stage/Enemies/";//後ろに"Enemies003.csv"のようにステージ番号をつける
    const std::string kEnemySpawnFilePrefix = "Enemies";
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

    //フェーズの遷移表
    const std::string kPhaseFlowDirectory = "data/Stage/PhaseFlow/";//後ろに"PhaseStage003.csv"
    const std::string kPhaseFlowFilePrefix = "PhaseStage";
    //#from,condition,param,to,event
    enum PhaseFlowColumn : int
    {
        From = 0,
        Condition,
        Param,
        To,
        Event,//空ならNone
    };
    constexpr int kPhaseFlowMinColumnNum = PhaseFlowColumn::Event;//event列は省略可

    //フェーズ進行のエリア
    const std::string kPhaseAreaDirectory = "data/Stage/PhaseAreas/";//後ろに"Stage003.csv"
    const char* const kPhaseAreaHeaderComment = "#name,posX,posY,posZ,halfExtentX,halfExtentY,halfExtentZ,rotYDeg";
    enum PhaseAreaColumn : int
    {
        AreaName = 0,
        AreaPosX,
        AreaPosY,
        AreaPosZ,
        AreaHalfExtentX,
        AreaHalfExtentY,
        AreaHalfExtentZ,
        AreaRotYDeg,
        PhaseAreaColumnNum,//列数
    };
}

void DataManager::LoadAll()
{
	//LoadPlayerAnimData();
	LoadBossAnimData();

    LoadAnimData(kPlayerAnimPath, m_playerAnimData);
    LoadAnimData(kEnemySwordmanAnimPath, m_enemySwordmanAnimData);
    LoadEnemySwordmanAttackRawData();
    LoadAnimData(kEnemyGunnerAnimPath, m_enemyGunnerAnimData);
    LoadEnemyGunnerAttackRawData();
    LoadAnimData(kEnemyBazookaAnimPath, m_enemyBazookaAnimData);
    LoadEnemyBazookaAttackRawData();
    LoadAnimData(kBossEnemyPirateAnimPath, m_bossEnemyPirateAnimData);
    LoadBossEnemyPirateAttackRawData();
    LoadAnimData(kBossArmAnimPath, m_bossArmAnimData);
    LoadEnemyStatusRawData();


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
        //3列目(animSpeed)//空欄なら1.0のまま
        auto speedComma = value.find(',');
        if (speedComma != std::string::npos)
        {
            std::string speed = value.substr(speedComma + 1);
            value = value.substr(0, speedComma);
            if (!speed.empty())animData.animSpeeds[key] = std::stof(speed);
        }
        animData.animNames[key] = value;
    }
}

void DataManager::LoadEnemySwordmanAnimData()
{
}

void DataManager::LoadEnemySwordmanAttackRawData()
{
    LoadRawCsv(kEnemySwordmanAttackPath, m_enemySwordmanAttackRawData);
}

void DataManager::LoadEnemyGunnerAttackRawData()
{
    LoadRawCsv(kEnemyGunnerAttackPath, m_enemyGunnerAttackRawData);
}

void DataManager::LoadEnemyBazookaAttackRawData()
{
    LoadRawCsv(kEnemyBazookaAttackPath, m_enemyBazookaAttackRawData);
}

void DataManager::ReloadBossEnemyPirateAnimData()
{
    //LoadAnimDataは追加するだけなので、消した行が残らないように空にしてから読む
    m_bossEnemyPirateAnimData = AnimData();
    LoadAnimData(kBossEnemyPirateAnimPath, m_bossEnemyPirateAnimData);
}

void DataManager::LoadBossEnemyPirateAttackRawData()
{
    LoadRawCsv(kBossEnemyPirateAttackPath, m_bossEnemyPirateAttackRawData);
}

void DataManager::LoadRawCsv(const std::string& filePath, std::vector<std::vector<std::string>>& rawData)
{
    std::ifstream file(filePath);
    assert(file.is_open() && "敵の攻撃CSVが開けませんでした");

    rawData.clear();//読み込み直しのときに前のデータが残らないように消す

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        rawData.push_back(Split(line, ','));//分割したトークンのベクターを生データとして保存する
    }
}

void DataManager::LoadEnemyStatusRawData()
{
    std::ifstream file(kEnemyStatusPath);
    assert(file.is_open() && "EnemyStatus.csvが開けませんでした");

    m_enemyStatusRawData.clear();//読み込み直しのときに前のデータが残らないように消す

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        m_enemyStatusRawData.push_back(Split(line, ','));//分割したトークンのベクターを生データとして保存する
    }
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
    oss << kEnemySpawnDirectory << kEnemySpawnFilePrefix << std::setw(3) << std::setfill('0') << stageNumber << ".csv";
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

std::string DataManager::GetPhaseFlowFilePath(int stageNumber)
{
    std::ostringstream oss;
    oss << kPhaseFlowDirectory << kPhaseFlowFilePrefix << std::setw(3) << std::setfill('0') << stageNumber << ".csv";
    return oss.str();
}

void DataManager::LoadPhaseFlow(int stageNumber)
{
    m_phaseFlow.clear();

    std::ifstream file(GetPhaseFlowFilePath(stageNumber));
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line))//1行ずつ読む
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();//改行コードの\rを消す
        if (line.empty() || line[0] == '#') continue;//空行・コメントは飛ばす

        auto tokens = Split(line, ',');//カンマで分ける
        if (tokens.size() < kPhaseFlowMinColumnNum) continue;//列不足は飛ばす

        PhaseTransition t;
        //コンディションを登録
        if (!StringToPhaseCondition(tokens[PhaseFlowColumn::Condition], t.condition)) continue;//知らない条件は飛ばす
        if (tokens.size() > PhaseFlowColumn::Event && !tokens[PhaseFlowColumn::Event].empty())//event列があれば
        {
            if (!StringToPhaseEvent(tokens[PhaseFlowColumn::Event], t.event)) continue;//知らないイベントは飛ばす
        }
        t.param = tokens[PhaseFlowColumn::Param];//条件の値
        //try catchは失敗するかもしれない処理を試して、失敗したら別の処理をする
        try
        {
            t.from = std::stoi(tokens[PhaseFlowColumn::From]);//元のフェーズ
            t.to = std::stoi(tokens[PhaseFlowColumn::To]);//次のフェーズ
            //Timer//KillCountは数字をもつ
            if (t.condition == PhaseConditionType::Timer || t.condition == PhaseConditionType::KillCount)
            {
                t.intParam = std::stoi(t.param);
            }

        }
        catch (...)
        {
            continue;//数字でなければ飛ばす
        }
        m_phaseFlow.push_back(t);//1行分を追加
    }
}

std::string DataManager::GetPhaseAreaFilePath(int stageNumber)
{
    std::ostringstream oss;
    oss << kPhaseAreaDirectory << "Stage" << std::setw(3) << std::setfill('0') << stageNumber << ".csv";
    return oss.str();
}

void DataManager::LoadPhaseAreas(int stageNumber)
{
    m_phaseAreas.clear();

    std::ifstream file(GetPhaseAreaFilePath(stageNumber));
    if (!file.is_open()) return;//無ければエリア無し

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();//改行コードの\rを消す
        if (line.empty() || line[0] == '#') continue;//空行・コメントは飛ばす

        auto tokens = Split(line, ',');
        if (tokens.size() < PhaseAreaColumn::PhaseAreaColumnNum) continue;//列不足は飛ばす

        //数値の欄が空欄・文字だとstd::stofの例外で止まる
        PhaseArea area;
        area.name = tokens[PhaseAreaColumn::AreaName];
        area.position = Vector3(std::stof(tokens[PhaseAreaColumn::AreaPosX]),
            std::stof(tokens[PhaseAreaColumn::AreaPosY]), std::stof(tokens[PhaseAreaColumn::AreaPosZ]));
        area.halfExtents = Vector3(std::stof(tokens[PhaseAreaColumn::AreaHalfExtentX]),
            std::stof(tokens[PhaseAreaColumn::AreaHalfExtentY]), std::stof(tokens[PhaseAreaColumn::AreaHalfExtentZ]));
        area.rotYDeg = std::stof(tokens[PhaseAreaColumn::AreaRotYDeg]);
        m_phaseAreas.push_back(area);
    }
}

bool DataManager::SavePhaseAreas(int stageNumber, const std::vector<PhaseArea>& areas)
{
    //出力先フォルダが無ければ作る
    std::filesystem::create_directories(kPhaseAreaDirectory);

    std::ofstream file(GetPhaseAreaFilePath(stageNumber));
    if (!file.is_open()) return false;

    file << kPhaseAreaHeaderComment << "\n";
    for (const auto& area : areas)
    {
        file << SanitizeCsvCell(area.name) << ","
            << area.position.x << "," << area.position.y << "," << area.position.z << ","
            << area.halfExtents.x << "," << area.halfExtents.y << "," << area.halfExtents.z << ","
            << area.rotYDeg << "\n";
    }
    if (!file.good()) return false;

    m_phaseAreas = areas;//保存した内容を持っているデータにも反映する
    return true;
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
    if (str == "Gunner") { outType = EnemyType::Gunner; return true; }
    if (str == "Bazooka") { outType = EnemyType::Bazooka; return true; }
    if (str == "BossEnemyPirate") { outType = EnemyType::BossEnemyPirate; return true; }
    if (str == "BossEnemyPirateR0") { outType = EnemyType::BossEnemyPirateR0; return true; }
    return false;
}

bool DataManager::StringToEnemyInitialState(const std::string& str, EnemyInitialState& outState)
{
    if (str == "Idle") { outState = EnemyInitialState::Idle; return true; }
    if (str == "Patrol") { outState = EnemyInitialState::Patrol; return true; }
    if (str == "Watch") { outState = EnemyInitialState::Watch; return true; }
    return false;
}

bool DataManager::StringToPhaseCondition(const std::string& str, PhaseConditionType& outCondition)
{
    if (str == "Start") { outCondition = PhaseConditionType::Start; return true; }
    if (str == "AllDead") { outCondition = PhaseConditionType::AllDead; return true; }
    if (str == "KillCount") { outCondition = PhaseConditionType::KillCount; return true; }
    if (str == "Timer") { outCondition = PhaseConditionType::Timer; return true; }
    if (str == "EnterArea") { outCondition = PhaseConditionType::EnterArea; return true; }
    return false;
}

bool DataManager::StringToPhaseEvent(const std::string& str, PhaseEvent& outEvent)
{
    if (str == "None") { outEvent = PhaseEvent::None; return true; }
    if (str == "BossAppear") { outEvent = PhaseEvent::BossAppear; return true; }
    if (str == "GameClear") { outEvent = PhaseEvent::GameClear; return true; }
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
