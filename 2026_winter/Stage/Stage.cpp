#include "Stage.h"
#include "../DataLoader/DataManager.h"
#include "../Math/Matrix4x4.h"
#include "Main/System.h"
#include "Collision/CollisionManager.h"
#include "Collision/PolygonShape.h"
#include <cmath>

namespace
{
	constexpr float kStageColOffsetX = 1200.0f;
	constexpr float kStageColOffsetY = 20.0f;

	const Vector3 kTitleStagePos = Vector3(27, -345.3f, -99);

	const Vector3 kTitleStageGraphicPos = Vector3(4800, -890, 0);//タイトルステージの描画位置オフセット
	constexpr float kTitleStageScale = 0.5f;//タイトルステージの描画スケール

	constexpr float kFloorCheckStartY = 800.0f;//床を調べる線分の開始高さ(モデル座標)//以前の配置(-800)でキャラが立っていた床を拾うため
	constexpr float kFloorCheckEndY = -100000.0f;//床を調べる線分の終了高さ(モデル座標)
	constexpr float kGameStageDefaultPosY = -800.0f;//床が見つからなかったときのステージの高さ

	//列車ステージのトンネル
	constexpr float kTunnelLength = 13200.0f;//トンネル1本の長さ(この長さで継ぎ目なくつながる)
	constexpr float kTunnelOffsetY = -150.0f;//トンネルのレール上面の高さ(列車のデッキの上面=0から見て)
	constexpr float kTunnelScrollSpeed = -20.0f;//1フレームに流す距離(列車の速さ)
	constexpr float kTunnelDrawRange = 6000.0f;//カメラから前後この距離までのトンネルを描く(SceneMainのファークリップより少し長く)

	//列車ステージの明るさ(戦う場所を明るく、トンネルと遠くを暗くする)
	constexpr float kTunnelColorScale = 0.45f;//トンネルの明るさ
	constexpr float kTrainColorScale = 1.15f;//列車の明るさ
	constexpr float kTrainFogStart = 2000.0f;//フォグがかかり始める距離
	constexpr float kTrainFogEnd = 5000.0f;//真っ暗になる距離(SceneMainのファークリップ)
	constexpr int kTrainFogColor = 8;//フォグの色(ほぼ黒)

	//ステージ1の夜のフォグ(近くはそのまま、遠くの建物を暗い青に沈める)
	constexpr float kNightFogStart = 1500.0f;
	constexpr float kNightFogEnd = 9000.0f;
	constexpr int kNightFogR = 10;
	constexpr int kNightFogG = 12;
	constexpr int kNightFogB = 24;
}


Stage::Stage()
{
	
}
void Stage::Init()
{
	// 当たり判定の初期化
	ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, 0, 0),
		.shape = std::make_unique<PolygonShape>(),
		.tag = {Collider::Faction::StaticObject, Collider::ColRole::None},
		.isActive = true
		});
}
void Stage::TitleInit()
{
	//モデルを別のもの
	m_stageViewHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::TitleStageModel));
	m_rb.m_pos = Vector3(0, -800, 0);

	MATRIX transmat_graphic = MGetTranslate(m_rb.m_pos.ToDxLibVector());

	MV1SetMatrix(m_stageViewHandle, transmat_graphic);
	MV1SetScale(m_stageViewHandle, VGet(kTitleStageScale, kTitleStageScale, kTitleStageScale));
}

void Stage::GameInit(const StageInfo& info)
{
	m_stageModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(info.colModel));
	m_stageViewHandle = MV1DuplicateModel(System::GetInstance().GetHandle(info.model));
	m_useNightFog = info.useNightFog;
	if (info.useTrainTunnel)
	{
		m_tunnelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::TrainTunnelModel));
		const COLOR_F tunnelScale = GetColorF(kTunnelColorScale, kTunnelColorScale, kTunnelColorScale, 1.0f);
		MV1SetAmbColorScale(m_tunnelHandle, tunnelScale);
		MV1SetDifColorScale(m_tunnelHandle, tunnelScale);
		//列車はマテリアルの環境光が明るさのほとんどなので、環境光だけ上げる(拡散は血殺のフォグが使う)
		MV1SetAmbColorScale(m_stageViewHandle, GetColorF(kTrainColorScale, kTrainColorScale, kTrainColorScale, 1.0f));
	}
	// モデルのポリゴンの当たり判定を構築する(第二引数を-1にすると全てのポリゴンを対象にする)
	MV1SetupCollInfo(m_stageModelHandle, -1);

	//原点の真下の床の高さを調べて、床がy=0に来るようにステージをずらす
	//(プレイヤー・敵・巡回ポイントはy=0基準で置いているため)
	float floorY = kGameStageDefaultPosY;
	MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(m_stageModelHandle, -1,
		VGet(0.0f, kFloorCheckStartY, 0.0f), VGet(0.0f, kFloorCheckEndY, 0.0f));
	if (hit.HitFlag)
	{
		floorY = -hit.HitPosition.y;
	}
	m_rb.m_pos = Vector3(0, floorY, 0);
	m_pos_graphic = Vector3(0, floorY, 0);
	auto polygonShape = dynamic_cast<PolygonShape*>(&GetShape());
	if (polygonShape)
	{
		polygonShape->SetModelHandle(m_stageModelHandle);
	}


	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);

	MATRIX transmat_graphic = MGetTranslate(m_pos_graphic.ToDxLibVector());

	MV1SetMatrix(m_stageModelHandle, Matrix4x4::ToDxLibMatrix(trans));
	MV1SetMatrix(m_stageViewHandle, transmat_graphic);
	MV1RefreshCollInfo(m_stageModelHandle, -1);//ずらした位置で当たり判定を作り直す
}

Stage::~Stage()
{
	MV1DeleteModel(m_stageModelHandle);
	MV1DeleteModel(m_stageViewHandle);
	if (m_tunnelHandle != -1)
	{
		MV1DeleteModel(m_tunnelHandle);
	}
	ClearWallZones();
}

void Stage::Update()
{
	// 毎フレーム衝突情報を更新
	MV1RefreshCollInfo(m_stageModelHandle, -1);

	if (m_tunnelHandle != -1)
	{
		//スローのときはトンネルもゆっくり流す
		m_tunnelScroll += kTunnelScrollSpeed * System::GetInstance().GetTimeScale();
		m_tunnelScroll = std::fmod(m_tunnelScroll, kTunnelLength);
	}
}

void Stage::Draw() const
{
	//MV1DrawModel(m_stageModelHandle);

	//列車ステージは遠くを暗くする(血殺のフォグがかかっているときはそちらを使う)
	const bool useTrainFog = m_tunnelHandle != -1 && !GetFogEnable();
	if (useTrainFog)
	{
		SetFogEnable(TRUE);
		SetFogMode(DX_FOGMODE_LINEAR);
		SetFogColor(kTrainFogColor, kTrainFogColor, kTrainFogColor);
		SetFogStartEnd(kTrainFogStart, kTrainFogEnd);
	}
	const bool useNightFog = m_useNightFog && !GetFogEnable();
	if (useNightFog)
	{
		SetFogEnable(TRUE);
		SetFogMode(DX_FOGMODE_LINEAR);
		SetFogColor(kNightFogR, kNightFogG, kNightFogB);
		SetFogStartEnd(kNightFogStart, kNightFogEnd);
	}

	MV1DrawModel(m_stageViewHandle);
	DrawTunnel();

	//キャラやエフェクトにはかけない
	if (useTrainFog || useNightFog)
	{
		SetFogEnable(FALSE);
	}
}

void Stage::DrawTunnel() const
{
	if (m_tunnelHandle == -1) return;

	//列車は-Zへ進む(機関車が-Z側、プレイヤーは+Zの最後尾へ進む)ので、トンネルは+Zへ流す
	//列車が長い(Z=-7900〜23400)ので、カメラの近くの分だけ描く
	const float camZ = GetCameraPosition().z - m_pos_graphic.z;
	const float halfLength = kTunnelLength * 0.5f;
	const int first = static_cast<int>(std::ceil((camZ - kTunnelDrawRange - halfLength - m_tunnelScroll) / kTunnelLength));
	const int last = static_cast<int>(std::floor((camZ + kTunnelDrawRange + halfLength - m_tunnelScroll) / kTunnelLength));
	for (int i = first; i <= last; ++i)
	{
		const float z = m_pos_graphic.z + i * kTunnelLength + m_tunnelScroll;
		MV1SetPosition(m_tunnelHandle, VGet(m_pos_graphic.x, m_pos_graphic.y + kTunnelOffsetY, z));
		MV1DrawModel(m_tunnelHandle);
	}
}

void Stage::OnCollision(Collider& other)
{
	//何もしない
}

void Stage::LoadWallZones(int stageNumber)
{
	ClearWallZones();

	//CSVの読み込みはDataManagerに任せ、読み込んだデータをゾーン(コライダー)に当てはめる
	auto& dataManager = DataManager::GetInstance();
	dataManager.LoadWallZoneData(stageNumber);
	for (const auto& data : dataManager.GetWallZoneData())
	{
		AddWallZone(data);
	}
}

bool Stage::SaveWallZones(int stageNumber) const
{
	return DataManager::GetInstance().SaveWallZoneData(stageNumber, GetAllWallZoneData());
}

std::vector<WallZoneData> Stage::GetAllWallZoneData() const
{
	std::vector<WallZoneData> zones;
	zones.reserve(m_wallZones.size());
	for (const auto& zone : m_wallZones)
	{
		zones.push_back(zone->GetData());
	}
	return zones;
}

void Stage::SetAllWallZoneData(const std::vector<WallZoneData>& zones)
{
	ClearWallZones();
	for (const auto& data : zones)
	{
		AddWallZone(data);
	}
}

bool Stage::IsInWallZone(Collider::ColRole role, const Collider& col) const
{
	for (const auto& zone : m_wallZones)
	{
		if (zone->GetRole() != role)continue;
		if (zone->IsOverlap(col))return true;
	}
	return false;
}

const WallZoneData& Stage::GetWallZoneData(int index) const
{
	return m_wallZones.at(index)->GetData();
}

void Stage::SetWallZoneData(int index, const WallZoneData& data)
{
	m_wallZones.at(index)->SetData(data);
}

int Stage::AddWallZone(const WallZoneData& data)
{
	auto zone = std::make_shared<WallZone>();
	zone->Init(data);
	m_wallZones.push_back(zone);
	return static_cast<int>(m_wallZones.size()) - 1;
}

void Stage::RemoveWallZone(int index)
{
	if (index < 0 || index >= static_cast<int>(m_wallZones.size()))return;
	CollisionManager::GetInstance().ReleaseCollider(m_wallZones[index]);
	m_wallZones.erase(m_wallZones.begin() + index);
}

bool Stage::IsWallZoneOverlap(int index, const Collider& col) const
{
	if (index < 0 || index >= static_cast<int>(m_wallZones.size()))return false;
	return m_wallZones[index]->IsOverlap(col);
}

void Stage::DrawWallZones(int selectedIndex) const
{
	for (int i = 0; i < static_cast<int>(m_wallZones.size()); ++i)
	{
		const auto& zone = m_wallZones[i];
		unsigned int color;
		if (i == selectedIndex)
		{
			color = GetColor(255, 255, 255);//選択中は白
		}
		else if (!zone->GetData().isActive)
		{
			color = GetColor(90, 90, 90);//無効は暗い灰色
		}
		else if (zone->GetRole() == Collider::ColRole::WallKickZone)
		{
			color = GetColor(255, 128, 0);//壁キックはオレンジ
		}
		else
		{
			color = GetColor(0, 200, 255);//壁走りは水色
		}
		zone->DrawWithColor(color);
	}
}

void Stage::ClearWallZones()
{
	for (auto& zone : m_wallZones)
	{
		CollisionManager::GetInstance().ReleaseCollider(zone);
	}
	m_wallZones.clear();
}
