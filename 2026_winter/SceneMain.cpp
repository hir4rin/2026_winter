#include "SceneMain.h"
#include "DxLib.h"
#include <cmath>
#include <unordered_map>
#include "Character/Player/Base/Player.h"
#include "Camera/Camera.h"
#include "Camera/CameraManager.h"
#include "DataLoader/DataManager.h"
#include "Managers/CollisionManager.h"
#include "Stage/Stage.h"
#include "Input.h"
#include "System.h"

namespace
{
	//グリッドの範囲と間隔
	constexpr float kGridRange = 500.0f;
	constexpr float kGridSpan = 100.0f;

	//軸線の長さ
	constexpr float kAxisLength = 700.0f;

	//色
	constexpr unsigned int kGridLineColor = 0x808080;//グレー
	constexpr unsigned int kAxisColorX = 0xff0000;//赤
	constexpr unsigned int kAxisColorY = 0x00ff00;//緑
	constexpr unsigned int kAxisColorZ = 0x0000ff;//青

	//プレイヤーのモデル
	const char* const kPlayerModelPath = "data/2026_winter_Player_noY.mv1";

	//カメラ設定
	constexpr float kCameraViewAngle = DX_PI_F / 3.0f;
	constexpr float kCameraNear = 100.0f;
	constexpr float kCameraFar = 5000.0f;
	const VECTOR kLightDir = { -1.0f, -1.0f, 1.0f };
}

SceneMain::SceneMain() :
	m_frameCount(0)
{
}

SceneMain::~SceneMain()
{
	//プレイヤー・ステージのコライダーをマネージャーから外してからモデルを解放する
	CollisionManager::GetInstance().Terminate();
	m_player.reset();
	m_stage.reset();
	if (m_lightHandle != -1)
	{
		DeleteLightHandle(m_lightHandle);
		m_lightHandle = -1;
	}
}

void SceneMain::Init()
{
	SetupCamera_Perspective(kCameraViewAngle);
	SetCameraNearFar(kCameraNear, kCameraFar);
	m_lightHandle = CreateDirLightHandle(VNorm(kLightDir));

	//CSVデータ(アニメーション名・コンボ)の読み込み
	DataManager::GetInstance().LoadAll();
	Input::GetInstance().Init();

	//Playerが使うモデルのハンドルを用意する(まだ無いもの(攻撃モデル・羽・武器・エフェクト)は-1にしておく)
	std::unordered_map<AsyncData, int> handles;
	for (int i = static_cast<int>(AsyncData::PlayerModel); i <= static_cast<int>(AsyncData::Goal); ++i)
	{
		handles[static_cast<AsyncData>(i)] = -1;
	}
	handles[AsyncData::PlayerModel] = MV1LoadModel(kPlayerModelPath);
	handles[AsyncData::TitleStageModel] = MV1LoadModel("data/Stage/TestStage/TestStage.mv1");
	System::GetInstance().SetHandleData(handles);

	CollisionManager::GetInstance().Init();
	

	//ステージの生成//当たり判定の初期化のみ行う(モデル・CSVの読み込みなどのデータ部分は別途対応する)
	m_stage = std::make_shared<Stage>();
	m_stage->Init();
	m_stage->GameInit();
	CollisionManager::GetInstance().RegisterCollider(m_stage);
	CollisionManager::GetInstance().SetStage(m_stage);

	//プレイヤーの生成
	m_player = std::make_shared<Player>();
	m_cameraManager = std::make_shared<CameraManager>();
	m_camera = std::make_unique<Camera>();
	m_player->SetCameraManager(m_cameraManager);
	m_player->Init();
	CollisionManager::GetInstance().RegisterCollider(m_player);

	//プレイヤー追従カメラ
	m_cameraManager->Init(m_player, m_stage);
}

void SceneMain::Update()
{
	m_frameCount++;

	Input::GetInstance().Update();
	m_player->Update(*m_camera);
	m_stage->Update();
	CollisionManager::GetInstance().Update();
	m_cameraManager->Update(m_player->GetRigidBody().GetPos());
}

void SceneMain::Draw()
{
	DrawGrid();

	m_player->Draw();
	m_stage->Draw();

	DrawFormatString(0, 0, GetColor(255, 255, 255), "FRAME:%d", m_frameCount);
}

void SceneMain::DrawGrid()
{
	//直線の始点と終点
	VECTOR startPos;
	VECTOR endPos;

	//XZ平面のグリッド線を描画する
	for (float z = -kGridRange; z <= kGridRange; z += kGridSpan)
	{
		startPos = VGet(-kGridRange, 0.0f, z);
		endPos = VGet(kGridRange, 0.0f, z);
		DrawLine3D(startPos, endPos, kGridLineColor);
	}
	for (float x = -kGridRange; x <= kGridRange; x += kGridSpan)
	{
		startPos = VGet(x, 0.0f, -kGridRange);
		endPos = VGet(x, 0.0f, kGridRange);
		DrawLine3D(startPos, endPos, kGridLineColor);
	}

	//X軸(赤)
	startPos = VGet(-kAxisLength, 0.0f, 0.0f);
	endPos = VGet(kAxisLength, 0.0f, 0.0f);
	DrawLine3D(startPos, endPos, kAxisColorX);

	//Y軸(緑)
	startPos = VGet(0.0f, -kAxisLength, 0.0f);
	endPos = VGet(0.0f, kAxisLength, 0.0f);
	DrawLine3D(startPos, endPos, kAxisColorY);

	//Z軸(青)
	startPos = VGet(0.0f, 0.0f, -kAxisLength);
	endPos = VGet(0.0f, 0.0f, kAxisLength);
	DrawLine3D(startPos, endPos, kAxisColorZ);
}
