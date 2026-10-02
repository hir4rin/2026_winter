#include "Stage.h"
#include "StageCsvIO.h"
#include "../Math/Matrix4x4.h"
#include "../System.h"
#include "../Managers/CollisionManager.h"
#include "../Collider/PolygonShape.h"

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

void Stage::GameInit()
{
	m_stageModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::TitleStageModel));
	m_stageViewHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::TitleStageModel));
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
	ClearStageObjects();
}

void Stage::Update()
{
	// 毎フレーム衝突情報を更新
	MV1RefreshCollInfo(m_stageModelHandle, -1);
}

void Stage::Draw() const
{
	//MV1DrawModel(m_stageModelHandle);
	MV1DrawModel(m_stageViewHandle);

	for (const auto& object : m_stageObjects)
	{
		object->Draw();
	}
}

void Stage::OnCollision(Collider& other)
{
	//何もしない
}

void Stage::LoadStageObjects(int stageNumber)
{
	ClearStageObjects();

	const std::vector<StageObjectData> objects = StageCsvIO::Load(stageNumber);
	m_stageObjects.reserve(objects.size());

	for (const auto& data : objects)
	{
		auto object = std::make_shared<StageObject>();
		object->Init(data.position, data.halfExtents);
		m_stageObjects.push_back(object);
	}
}

void Stage::ClearStageObjects()
{
	for (auto& object : m_stageObjects)
	{
		CollisionManager::GetInstance().ReleaseCollider(object);
	}
	m_stageObjects.clear();
}