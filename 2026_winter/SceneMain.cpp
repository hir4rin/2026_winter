#include "SceneMain.h"
#include "Scene/SceneController.h"
#include "DxLib.h"
#include <cmath>
#include <unordered_map>
#include "Character/Player/Base/Player.h"
#include "Character/Enemy/EnemySwordman.h"
#include "Character/Enemy/EnemyManager.h"
#include "Camera/Camera.h"
#include "Camera/CameraManager.h"
#include "Camera/LockOnManager.h"
#include "DataLoader/DataManager.h"
#include "Managers/CollisionManager.h"
#include "Stage/Stage.h"
#include "Input.h"
#include "System.h"
#include "BattleManager.h"
#include "Game.h"

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

	//カメラ設定
	constexpr float kCameraViewAngle = DX_PI_F / 3.0f;
	constexpr float kCameraNear = 100.0f;
	constexpr float kCameraFar = 5000.0f;
	const VECTOR kLightDir = { -1.0f, -1.0f, 1.0f };

	//フェードにかけるフレーム数
	constexpr int kFadeFrame = 30;

	//敵の出現位置
	const Vector3 kEnemyStartPos = Vector3(0.0f, 0.0f, 500.0f);
}

SceneMain::SceneMain(SceneController& controller) :
	Scene(controller),
	m_frameCount(0),
	m_fadeFrame(0)
{
	m_updateFunc = static_cast<UpdateFunc_t>(&SceneMain::NormalUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&SceneMain::NormalDraw);
	Init();
}

SceneMain::~SceneMain()
{
	//プレイヤー・ステージのコライダーをマネージャーから外してからモデルを解放する
	CollisionManager::GetInstance().Terminate();
	m_player.reset();
	m_enemy.reset();
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

	CollisionManager::GetInstance().Init();

	//エネミーマネージャー
	m_enemyManager = std::make_shared<EnemyManager>();

	//ステージの生成//当たり判定の初期化のみ行う(モデル・CSVの読み込みなどのデータ部分は別途対応する)
	m_stage = std::make_shared<Stage>();
	m_stage->Init();
	m_stage->GameInit();
	CollisionManager::GetInstance().SetStage(m_stage);

	//プレイヤーの生成
	m_player = std::make_shared<Player>();
	m_cameraManager = std::make_shared<CameraManager>();
	m_camera = std::make_unique<Camera>();
	m_player->SetCameraManager(m_cameraManager);
	m_player->SetEnemyManager(m_enemyManager);
	m_player->Init();


	//敵の生成
	m_enemy = std::make_shared<EnemySwordman>(m_player, kEnemyStartPos);
	m_enemy->Init();
	m_enemyManager->AddEnemy(m_enemy);//追加


	m_battleManager = std::make_shared<BattleManager>();
	System::GetInstance().SetBattleMgr(m_battleManager);

	//プレイヤー追従カメラ
	m_cameraManager->Init(m_player, m_stage);
}

void SceneMain::Update()
{
	(this->*m_updateFunc)();
}

void SceneMain::Draw()
{
	(this->*m_drawFunc)();
}

void SceneMain::FadeInUpdate()
{
	NormalUpdate();
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&SceneMain::NormalUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&SceneMain::NormalDraw);
	}
}

void SceneMain::NormalUpdate()
{
	m_frameCount++;

	Input::GetInstance().SetInputBlocked(m_battleManager->GetIsEventPlaying());
	Input::GetInstance().Update();
	m_player->Update(*m_camera);
	m_enemyManager->Update();
	m_stage->Update();
	CollisionManager::GetInstance().Update();
	//battleManagerをここで直でするか、Systemの中でするかどちらがいいか気になる
	System::GetInstance().Update();

	m_cameraManager->Update();
}

void SceneMain::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		//TODO:次のシーンができたらここで切り替える
		//m_controller.ResetScene<次のシーン>();
		m_fadeFrame = kFadeFrame;
	}
}

void SceneMain::FadeInDraw()
{
	NormalDraw();
	DrawFade();
}

void SceneMain::NormalDraw()
{
	DrawGrid();

	m_player->Draw();
	m_enemyManager->Draw();
	m_stage->Draw();
#ifdef _DEBUG
	CollisionManager::GetInstance().DebugDraw();
	DrawFormatString(0, 0, GetColor(255, 255, 255), "FRAME:%d", m_frameCount);
#endif
}

void SceneMain::FadeOutDraw()
{
	NormalDraw();
	DrawFade();
}

void SceneMain::DrawFade()
{
	//フェードイン中は黒→透明、フェードアウト中は透明→黒
	float rate = static_cast<float>(m_fadeFrame) / static_cast<float>(kFadeFrame);
	if (m_updateFunc == static_cast<UpdateFunc_t>(&SceneMain::FadeInUpdate))
	{
		rate = 1.0f - rate;
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, 0x000000, true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
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
