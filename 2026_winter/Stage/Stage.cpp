#include "Stage.h"
#include "../DataLoader/DataManager.h"
#include "../Math/Matrix4x4.h"
#include "Main/System.h"
#include "Collision/CollisionManager.h"
#include "Collision/PolygonShape.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

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
	constexpr float kKillYMargin = 500.0f;//当たり判定のモデルの一番下からどれだけ下で落下死にするか

	//列車ステージのトンネル
	constexpr float kTunnelLength = 13200.0f;//トンネル1本の長さ(この長さで継ぎ目なくつながる)
	constexpr float kTunnelOffsetY = -150.0f;//トンネルのレール上面の高さ(列車のデッキの上面=0から見て)
	constexpr float kTunnelScrollSpeed = -20.0f;//1フレームに流す距離(列車の速さ)
	constexpr float kTunnelDrawRange = 6000.0f;//カメラから前後この距離までのトンネルを描く(SceneMainのファークリップより少し長く)
	constexpr float kTunnelExitSizeRate = 0.85f;//出口の穴の大きさ(トンネルの外側の大きさに対して)
	constexpr int kTunnelExitArchDiv = 24;//出口の丸い所の分け方
	constexpr float kTunnelExitHaloRate = 1.15f;//出口のまわりの光のにじみの大きさ
	constexpr int kTunnelExitHaloAlpha = 90;
	constexpr float kTrackLampLightRange = 2500.0f;//プレイヤーからこの距離までの線路脇の灯りを明かりにする(TrainLighting の kActiveDistance)

	//列車ステージの明るさ(戦う場所を明るく、トンネルと遠くを暗くする)
	constexpr float kTunnelColorScale = 0.45f;//トンネルの明るさ
	constexpr float kTrackColorScale = 0.7f;//トンネルの外の線路の明るさ
	constexpr float kTrainColorScale = 1.15f;//列車の明るさ
	constexpr float kTrainFogStart = 1500.0f;//フォグがかかり始める距離
	constexpr float kTrainFogEnd = 5000.0f;//真っ暗になる距離(SceneMainのファークリップ)
	constexpr int kTrainFogR = 3;//フォグの色(紺に近い黒)
	constexpr int kTrainFogG = 5;
	constexpr int kTrainFogB = 12;

	//ステージ1の夜のフォグ(近くはそのまま、遠くの建物を空の地平線の色に沈める。カメラのファー5000より手前で消しきる)
	constexpr float kNightFogStart = 2200.0f;
	constexpr float kNightFogEnd = 4950.0f;
	constexpr float kFogOffStart = 100000.0f;//フォグを切るときの距離
	constexpr float kFogOffEnd = 200000.0f;
	constexpr int kNightFogR = 48;
	constexpr int kNightFogG = 24;
	constexpr int kNightFogB = 62;
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
	BuildMeshBoxes();
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
		//置いた位置から見た、進む向きの端(モデルのフレームがずれているので実際に測る)
		MV1SetPosition(m_tunnelHandle, VGet(0.0f, 0.0f, 0.0f));
		MV1SetupReferenceMesh(m_tunnelHandle, -1, TRUE);
		const MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(m_tunnelHandle, -1, TRUE);
		m_tunnelFrontOffset = (GetTrainForwardZ() > 0.0f) ? ref.MaxPosition.z : ref.MinPosition.z;
		m_tunnelHalfWidth = (ref.MaxPosition.x - ref.MinPosition.x) * 0.5f;
		m_tunnelMinY = ref.MinPosition.y;
		m_tunnelMaxY = ref.MaxPosition.y;
		MV1TerminateReferenceMesh(m_tunnelHandle, -1, TRUE);
		//トンネルの外の線路(トンネルと同じ位置に置けばレールがつながる)
		m_trackHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::TrainTrackModel));
		if (m_trackHandle != -1)
		{
			const COLOR_F trackScale = GetColorF(kTrackColorScale, kTrackColorScale, kTrackColorScale, 1.0f);
			MV1SetAmbColorScale(m_trackHandle, trackScale);
			MV1SetDifColorScale(m_trackHandle, trackScale);
		}
		//列車はマテリアルの環境光が明るさのほとんどなので、環境光だけ上げる
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
	BuildMeshBoxes();

	//当たり判定のモデルの一番下より下に落ちたら落下死
	MV1SetupReferenceMesh(m_stageModelHandle, -1, TRUE);
	MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(m_stageModelHandle, -1, TRUE);
	m_killY = ref.MinPosition.y - kKillYMargin;
	MV1TerminateReferenceMesh(m_stageModelHandle, -1, TRUE);

	//明かりのCSV(番号は敵配置と同じ。無いステージは何もしない)
	m_lights.Load(info.enemySpawnNumber, m_pos_graphic);
	if (m_useNightFog) m_lights.SetFogRange(kNightFogStart, kNightFogEnd);

	//列車ステージのライティング(トンネルと一緒に流れる明かり・車内の暖色)
	if (m_tunnelHandle != -1)
	{
		m_trainLighting = std::make_unique<TrainLighting>();
		m_trainLighting->Init(m_tunnelHandle, m_pos_graphic, kTunnelOffsetY);
		m_trainSparks = std::make_unique<TrainSparks>();
		m_trainSparks->Init(m_pos_graphic);
		m_outside = std::make_unique<TrainOutside>();
		m_outside->Init(m_pos_graphic, kTunnelOffsetY, GetTrainForwardZ());
	}

	//扉(無いステージは何もしない)
	LoadGates(info.enemySpawnNumber);
}

Stage::~Stage()
{
	m_trainLighting.reset();//キャラの光・空気感を戻す
	m_trainSparks.reset();//再生中の火花を止めてエフェクトを消す
	m_outside.reset();
	MV1DeleteModel(m_stageModelHandle);
	MV1DeleteModel(m_stageViewHandle);
	if (m_tunnelHandle != -1)
	{
		MV1DeleteModel(m_tunnelHandle);
	}
	if (m_trackHandle != -1)
	{
		MV1DeleteModel(m_trackHandle);
	}
	ClearWallZones();
	ClearGates();
}

void Stage::Update()
{
	// 毎フレーム衝突情報を更新
	MV1RefreshCollInfo(m_stageModelHandle, -1);

	if (m_tunnelHandle != -1)
	{
		//スローのときはトンネルもゆっくり流す
		const float move = kTunnelScrollSpeed * m_tunnelSpeedScale * System::GetInstance().GetTimeScale();
		m_tunnelScroll += move;
		m_tunnelScroll = std::fmod(m_tunnelScroll, kTunnelLength);
		if (m_hasTunnelExit) m_tunnelExitZ += move;
	}
}

float Stage::GetTunnelSpeed() const
{
	return std::fabs(kTunnelScrollSpeed) * m_tunnelSpeedScale;
}

float Stage::GetTrainForwardZ() const
{
	//トンネルが流れる向きの逆へ進む
	return (kTunnelScrollSpeed < 0.0f) ? 1.0f : -1.0f;
}

Stage::FogSetting Stage::GetTrainFog() const
{
	return { kTrainFogR, kTrainFogG, kTrainFogB, kTrainFogStart, kTrainFogEnd };
}

float Stage::StartTunnelExit(float baseZ, float minDistance)
{
	if (m_tunnelHandle == -1) return 0.0f;
	//minDistance 先より向こうで、一番近いトンネルの継ぎ目を出口にする
	const float forward = GetTrainForwardZ();
	const float target = baseZ + forward * minDistance;
	const float origin = m_pos_graphic.z + m_tunnelScroll + m_tunnelFrontOffset;
	const float index = forward > 0.0f ? std::ceil((target - origin) / kTunnelLength) : std::floor((target - origin) / kTunnelLength);
	m_tunnelExitZ = origin + index * kTunnelLength;
	m_hasTunnelExit = true;
	return GetTunnelExitDistance(baseZ);
}

float Stage::GetTunnelExitDistance(float z) const
{
	if (!m_hasTunnelExit) return 1.0e9f;
	return (m_tunnelExitZ - z) * GetTrainForwardZ();
}

void Stage::UpdateLights(const Vector3& playerPos)
{
	m_lights.Update(playerPos);
	if (m_trainLighting && m_hasTunnelExit && m_outside)
	{
		//出口より先のトンネルの明かりを消し、線路脇の灯りを流す
		m_trainLighting->SetTunnelExit(m_tunnelExitZ, GetTrainForwardZ());
		m_outside->GetLampLights(m_tunnelExitZ, playerPos.z, kTrackLampLightRange, m_trackLampBuffer);
		m_trainLighting->SetTrackLamps(m_trackLampBuffer);
	}
	if (m_trainLighting) m_trainLighting->Update(playerPos, m_tunnelScroll, kTunnelLength);
	if (m_trainSparks) m_trainSparks->Update(playerPos);
}

void Stage::DrawLightDebugWindow()
{
	m_lights.DrawDebugWindow();
	if (m_trainLighting) m_trainLighting->DrawDebugWindow();
}

void Stage::Draw() const
{
	//MV1DrawModel(m_stageModelHandle);

	//列車ステージは遠くを暗くする
	const bool useTrainFog = m_tunnelHandle != -1;
	if (useTrainFog)
	{
		SetFogEnable(TRUE);
		SetFogMode(DX_FOGMODE_LINEAR);
		const FogSetting fog = m_hasFogOverride ? m_fogOverride : GetTrainFog();
		SetFogColor(fog.r, fog.g, fog.b);
		SetFogStartEnd(fog.start, fog.end);
	}
	const bool useNightFog = m_useNightFog;
	if (useNightFog)
	{
		SetFogEnable(TRUE);
		SetFogMode(DX_FOGMODE_LINEAR);
		SetFogColor(kNightFogR, kNightFogG, kNightFogB);
		SetFogStartEnd(kNightFogStart, kNightFogEnd);
	}

	//抜けた後の夜の空と、出口の光は背景として先に描く(出口より先の列車も上に重なって見える)
	if (m_outside) m_outside->DrawSky();
	DrawTunnelExitLight();

	//いつもの光＋明かりを1回で描く(明かりの無いステージはいつもの描画)
	CullMeshes();
	ModelShaderManager::GetInstance().DrawModel(m_stageViewHandle);
	for (auto& gate : m_gates)
	{
		gate->Draw();
	}
	DrawTunnel();
	if (m_outside && m_hasTunnelExit) m_outside->DrawLamps(m_tunnelExitZ);

	//キャラやエフェクトにはかけない
	if (useTrainFog || useNightFog)
	{
		//自作の頂点シェーダーはフォグを切っても距離の値を読むので、遠くへ送ってから切る
		SetFogStartEnd(kFogOffStart, kFogOffEnd);
		SetFogEnable(FALSE);
	}
	if (m_outside && m_hasTunnelExit) m_outside->DrawGlows(m_tunnelExitZ);
}

void Stage::SetOutsideRate(float rate)
{
	if (m_outside) m_outside->SetRate(rate);
	if (m_trainLighting) m_trainLighting->SetOutsideRate(rate);
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
		//出口より先はトンネルの代わりに線路だけ描く
		const bool isOutside = m_hasTunnelExit && GetTunnelExitDistance(z + m_tunnelFrontOffset) < -1.0f;
		const int handle = isOutside ? m_trackHandle : m_tunnelHandle;
		if (handle == -1) continue;
		MV1SetPosition(handle, VGet(m_pos_graphic.x, m_pos_graphic.y + kTunnelOffsetY, z));
		ModelShaderManager::GetInstance().DrawModel(handle);
	}
}

void Stage::DrawTunnelExitLight() const
{
	if (!m_hasTunnelExit || GetTunnelExitDistance(GetCameraPosition().z) <= 0.0f) return;
	//出口の穴(上が丸いアーチ)を明るい色で塗る
	const float bottom = m_pos_graphic.y + kTunnelOffsetY + m_tunnelMinY;
	const float spring = m_pos_graphic.y;//ここから上を丸くする
	const float halfWidth = m_tunnelHalfWidth * kTunnelExitSizeRate;
	const float archHeight = (m_pos_graphic.y + kTunnelOffsetY + m_tunnelMaxY - spring) * kTunnelExitSizeRate;
	const float centerX = m_pos_graphic.x;
	const unsigned int color = GetColor(m_exitLightR, m_exitLightG, m_exitLightB);
	SetFogEnable(FALSE);
	SetUseLighting(FALSE);
	SetUseBackCulling(FALSE);//どちらから見ても描く
	SetUseZBuffer3D(FALSE);//後から描くステージが上に重なるように
	SetWriteZBuffer3D(FALSE);
	//外側にうっすら光のにじみ、その内側に穴
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, kTunnelExitHaloAlpha);
	DrawArch(centerX, bottom, spring, halfWidth * kTunnelExitHaloRate, archHeight * kTunnelExitHaloRate, color);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawArch(centerX, bottom, spring, halfWidth, archHeight, color);
	SetWriteZBuffer3D(TRUE);//Application::Init で有効にしている
	SetUseZBuffer3D(TRUE);
	SetUseBackCulling(TRUE);
	SetUseLighting(TRUE);
	SetFogEnable(TRUE);
}

void Stage::DrawArch(float centerX, float bottom, float spring, float halfWidth, float archHeight, unsigned int color) const
{
	const VECTOR baseL = VGet(centerX - halfWidth, bottom, m_tunnelExitZ);
	const VECTOR baseR = VGet(centerX + halfWidth, bottom, m_tunnelExitZ);
	const VECTOR springL = VGet(centerX - halfWidth, spring, m_tunnelExitZ);
	const VECTOR springR = VGet(centerX + halfWidth, spring, m_tunnelExitZ);
	DrawTriangle3D(baseL, baseR, springR, color, TRUE);
	DrawTriangle3D(baseL, springR, springL, color, TRUE);
	const VECTOR center = VGet(centerX, spring, m_tunnelExitZ);
	for (int i = 0; i < kTunnelExitArchDiv; ++i)
	{
		const float a0 = DX_PI_F * i / kTunnelExitArchDiv;
		const float a1 = DX_PI_F * (i + 1) / kTunnelExitArchDiv;
		const VECTOR p0 = VGet(centerX + cosf(a0) * halfWidth, spring + sinf(a0) * archHeight, m_tunnelExitZ);
		const VECTOR p1 = VGet(centerX + cosf(a1) * halfWidth, spring + sinf(a1) * archHeight, m_tunnelExitZ);
		DrawTriangle3D(center, p0, p1, color, TRUE);
	}
}

void Stage::BuildMeshBoxes()
{
	m_meshBoxes.assign(MV1GetMeshNum(m_stageViewHandle), MeshBox{});
	for (int frame = 0; frame < MV1GetFrameNum(m_stageViewHandle); ++frame)
	{
		const MATRIX mat = MV1GetFrameLocalWorldMatrix(m_stageViewHandle, frame);
		for (int i = 0; i < MV1GetFrameMeshNum(m_stageViewHandle, frame); ++i)
		{
			const int mesh = MV1GetFrameMesh(m_stageViewHandle, frame, i);
			const VECTOR localMin = MV1GetMeshMinPosition(m_stageViewHandle, mesh);
			const VECTOR localMax = MV1GetMeshMaxPosition(m_stageViewHandle, mesh);
			//8つの角をワールドに直して、それを囲む箱にする
			MeshBox box = { VTransform(localMin, mat), VTransform(localMin, mat) };
			for (int corner = 1; corner < 8; ++corner)
			{
				const VECTOR local = VGet((corner & 1) ? localMax.x : localMin.x, (corner & 2) ? localMax.y : localMin.y, (corner & 4) ? localMax.z : localMin.z);
				const VECTOR world = VTransform(local, mat);
				box.min = VGet((std::min)(box.min.x, world.x), (std::min)(box.min.y, world.y), (std::min)(box.min.z, world.z));
				box.max = VGet((std::max)(box.max.x, world.x), (std::max)(box.max.y, world.y), (std::max)(box.max.z, world.z));
			}
			m_meshBoxes[mesh] = box;
		}
	}
}

void Stage::CullMeshes()const
{
	//今のカメラの視界に箱が少しでも入っていれば描く
	for (int mesh = 0; mesh < static_cast<int>(m_meshBoxes.size()); ++mesh)
	{
		const bool isOutside = CheckCameraViewClip_Box(m_meshBoxes[mesh].min, m_meshBoxes[mesh].max) != FALSE;
		MV1SetMeshVisible(m_stageViewHandle, mesh, isOutside ? FALSE : TRUE);
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

const WallZoneData* Stage::FindWallZoneData(const std::string& name) const
{
	for (const auto& zone : m_wallZones)
	{
		if (zone->GetData().name == name) return &zone->GetData();
	}
	return nullptr;
}

bool Stage::IsInWallZoneNamed(const std::string& name, const Collider& col) const
{
	for (const auto& zone : m_wallZones)
	{
		if (zone->GetData().name == name && zone->IsOverlap(col)) return true;
	}
	return false;
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
		else if (zone->GetRole() == Collider::ColRole::DeathZone)
		{
			color = GetColor(255, 0, 0);//落下死は赤
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

void Stage::LoadGates(int stageNumber)
{
	ClearGates();
	char path[64];
	snprintf(path, sizeof(path), "data/Stage/Gates/Stage%03d.csv", stageNumber);
	std::ifstream file(path);
	if (!file.is_open()) return;

	//name,model,pos(3),rotYDeg,closePhase,openPhase,delayFrame,moveFrame,move(3),scale(3),colOffset(3),colHalf(3)[,size(3)]
	constexpr int kColumnNum = 22;
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty() || line[0] == '#') continue;

		std::istringstream ss(line);
		std::string token;
		std::vector<std::string> t;
		while (std::getline(ss, token, ','))
		{
			t.push_back(token);
		}
		if (t.size() < kColumnNum) continue;

		GateData data;
		try
		{
			auto vec = [&](int i) { return Vector3(std::stof(t[i]), std::stof(t[i + 1]), std::stof(t[i + 2])); };
			data.name = t[0];
			data.model = t[1];
			data.pos = vec(2);
			data.rotYDeg = std::stof(t[5]);
			data.closePhase = std::stoi(t[6]);
			data.openPhase = std::stoi(t[7]);
			data.delayFrame = std::stoi(t[8]);
			data.moveFrame = std::stoi(t[9]);
			data.openMove = vec(10);
			data.openScale = vec(13);
			data.colOffset = vec(16);
			data.colHalf = vec(19);
			if (t.size() >= kColumnNum + 3) data.size = vec(22);
		}
		catch (...)
		{
			continue;//数値にできない行は飛ばす
		}
		auto gate = std::make_shared<Gate>();
		gate->Init(data, m_pos_graphic);
		m_gates.push_back(gate);
	}
}

void Stage::ClearGates()
{
	for (auto& gate : m_gates)
	{
		CollisionManager::GetInstance().ReleaseCollider(gate);
	}
	m_gates.clear();
}

void Stage::UpdateGates(int currentPhase)
{
	for (auto& gate : m_gates)
	{
		gate->Update(currentPhase);
	}
}

MV1_COLL_RESULT_POLY Stage::CollCheckCameraLine(const Vector3& start, const Vector3& end) const
{
	MV1_COLL_RESULT_POLY best = MV1CollCheck_Line(m_stageModelHandle, -1, start.ToDxLibVector(), end.ToDxLibVector());
	float bestDist = best.HitFlag ? (Vector3::FromDxLibVector(best.HitPosition) - start).sqMagnitude() : -1.0f;
	for (auto& gate : m_gates)
	{
		MV1_COLL_RESULT_POLY hit;
		if (!gate->CollCheckCameraLine(start, end, hit)) continue;
		const float dist = (Vector3::FromDxLibVector(hit.HitPosition) - start).sqMagnitude();
		if (bestDist < 0.0f || dist < bestDist)
		{
			best = hit;
			bestDist = dist;
		}
	}
	return best;
}

void Stage::DrawGateCols() const
{
	for (auto& gate : m_gates)
	{
		gate->DrawCol(gate->GetIsActive() ? GetColor(255, 60, 60) : GetColor(80, 255, 80));
		gate->DrawCameraBox(GetColor(80, 200, 255));
	}
}
