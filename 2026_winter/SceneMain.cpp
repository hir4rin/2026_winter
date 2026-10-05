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
#include "imguiApp.h"
#include "Scene/PauseScene.h"
#include "Stage/WallZoneEditor.h"
#include "imgui.h"

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
	//ステージ用ライトの明るさ(0.0~1.0) ※カメラ追従ライト(Camera.cpp)と合算されるので控えめにする
	constexpr float kLightDifColorValue = 0.4f;//ディフューズ
	constexpr float kLightSpcColorValue = 0.2f;//スペキュラ
	constexpr float kLightAmbColorValue = 0.0f;//アンビエント(カメラ側で設定済み)

	//フェードにかけるフレーム数
	constexpr int kFadeFrame = 30;

	//敵の出現位置
	const Vector3 kEnemyStartPos = Vector3(0.0f, 0.0f, 500.0f);

	//スクリーンショットの保存先
	const char* const kScreenshotPath = "data/SaveData/screenShot.png";
}

SceneMain::SceneMain(SceneController& controller, StageType stageType) :
	Scene(controller),
	m_frameCount(0),
	m_fadeFrame(0),
	m_stageType(stageType)
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
	m_stage.reset();
	if (m_lightHandle != -1)
	{
		DeleteLightHandle(m_lightHandle);
		m_lightHandle = -1;
	}
	//標準ライトを元に戻す(他シーンに影響させないため)
	SetLightEnable(TRUE);
}

void SceneMain::Init()
{
	SetupCamera_Perspective(kCameraViewAngle);
	SetCameraNearFar(kCameraNear, kCameraFar);
	//DxLibの標準ライトは他のライトと重なって明るくなりすぎるので無効化
	//SetLightEnable(FALSE);
	m_lightHandle = CreateDirLightHandle(VNorm(kLightDir));
	SetLightDifColorHandle(m_lightHandle, GetColorF(kLightDifColorValue, kLightDifColorValue, kLightDifColorValue, 1.0f));
	SetLightSpcColorHandle(m_lightHandle, GetColorF(kLightSpcColorValue, kLightSpcColorValue, kLightSpcColorValue, 1.0f));
	SetLightAmbColorHandle(m_lightHandle, GetColorF(kLightAmbColorValue, kLightAmbColorValue, kLightAmbColorValue, 1.0f));

	//CSVデータ(アニメーション名・コンボ)の読み込み
	DataManager::GetInstance().LoadAll();
	Input::GetInstance().Init();

	CollisionManager::GetInstance().Init();

	//エネミーマネージャー
	m_enemyManager = std::make_shared<EnemyManager>();

	//ステージの生成//当たり判定の初期化のみ行う(モデル・CSVの読み込みなどのデータ部分は別途対応する)
	m_stage = std::make_shared<Stage>();
	m_stage->Init();
	const StageInfo& stageInfo = StageData::GetInfo(m_stageType);
	m_stage->GameInit(stageInfo.model);
	CollisionManager::GetInstance().SetStage(m_stage);
	//ステージ編集モードで保存した壁キック/壁走りゾーンを読み込む
	m_stage->LoadWallZones(stageInfo.wallZoneNumber);
	m_wallZoneEditor = std::make_unique<WallZoneEditor>();
	m_wallZoneEditor->SetStageNumber(stageInfo.wallZoneNumber);

	//プレイヤーの生成
	m_player = std::make_shared<Player>();
	m_cameraManager = std::make_shared<CameraManager>();
	m_camera = std::make_unique<Camera>();
	m_player->SetCameraManager(m_cameraManager);
	m_player->SetEnemyManager(m_enemyManager);
	m_player->Init();


	//敵の生成
	m_enemyManager->SetPlayer(m_player);
	//Unityで書き出した敵配置CSVを読み込んで、最初のフェーズの敵を出す
	DataManager::GetInstance().LoadEnemySpawnData(stageInfo.enemySpawnNumber);
	const auto& spawnData = DataManager::GetInstance().GetEnemySpawnData();
	if (!spawnData.empty())
	{
		m_enemyManager->StartSpawn(spawnData);
	}
	else
	{
		//配置データが無いステージは、今まで通りテスト用の敵を1体出す
		//EnemyManagerだけが所有する(ここで持ち続けると、死体を消しても実体が残ってしまう)
		auto enemy = std::make_shared<EnemySwordman>(m_player, kEnemyStartPos);
		enemy->Init();
		m_enemyManager->AddEnemy(enemy);//追加
	}


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

	auto& input = Input::GetInstance();
	auto battleMgr = System::GetInstance().GetBattleMgr();

	//ステージ編集中にImGuiの文字入力をしているときは、キー入力をゲーム側で使わない
	//(名前の入力中にPキーでポーズが開いたり、矢印キーでカメラが動いたりしないように)
	const bool isImGuiTyping = battleMgr->GetStageEditMode() && ImGui::GetIO().WantCaptureKeyboard;

	//Startでポーズシーンを積む
	if (input.IsTriggered("Start") && !isImGuiTyping)
	{
		//フォトモード・ステージ編集中はフリーカメラの位置を保持したままにする
		if (!battleMgr->GetPhotoMode() && !battleMgr->GetStageEditMode())
		{
			//今のカメラの座標・角度をフォトカメラに保存しておく
			m_cameraManager->GetActiveCamera()->Exit();
			m_cameraManager->SetPhotoCamera();
		}
		m_controller.PushScene(std::make_shared<PauseScene>(m_controller));
		return;
	}

	//Wキーを押したらタイムスケールを0.1にする
	if (CheckHitKey(KEY_INPUT_W))
	{
		if (System::GetInstance().GetTimeScale() != 0.1f)
		{
			System::GetInstance().SetTimeScale(0.1f);
		}
		else
		{
			System::GetInstance().SetTimeScale(1.0f);
		}
	}


	//フォトモード中はカメラだけを動かす
	if (battleMgr->GetPhotoMode())
	{
		m_cameraManager->UpdatePhotoCamera();
		m_cameraManager->ApplyCameraSettings();

		if (input.IsTriggered("Y"))
		{
			//今フレームの描画はまだ終わっていないので、保存はDrawの最後で行う
			m_requestScreenshot = true;
		}
		return;
	}

	//ステージ編集中もゲームは止めて、フリーカメラだけを動かす(ゾーンの編集はDrawのImGuiウィンドウで行う)
	if (battleMgr->GetStageEditMode())
	{
		if (!isImGuiTyping)
		{
			m_cameraManager->UpdatePhotoCamera();
		}
		m_cameraManager->ApplyCameraSettings();
		return;
	}

	m_player->Update(*m_camera);
	m_enemyManager->Update();
	m_stage->Update();
	CollisionManager::GetInstance().Update();
	//battleManagerをここで直でするか、Systemの中でするかどちらがいいか気になる
	System::GetInstance().Update();

	m_cameraManager->Update();

	//再生中のエフェクトを進める
	UpdateEffekseer3D();
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

	//DxLibのカメラ設定をEffekseerに反映してからエフェクトを描画する
	Effekseer_Sync3DSetting();
	DrawEffekseer3D();
#ifdef _DEBUG
	CollisionManager::GetInstance().DebugDraw();
	DrawFormatString(0, 0, GetColor(255, 255, 255), "FRAME:%d", m_frameCount);
#endif

	//フォトモード中はカメラ調整用のImGuiウィンドウを出す(F2で表示/非表示)
	if (System::GetInstance().GetBattleMgr()->GetPhotoMode())
	{
		imguiApp::GetInstance().DrawCameraDebugWindow(
			m_cameraManager->GetPhotoCameraPos(),
			m_cameraManager->GetPhotoCameraTarget());
		imguiApp::GetInstance().DrawCameraAnimatorWindow(
			m_cameraManager->GetPhotoCameraPos(),
			m_cameraManager->GetPhotoCameraTarget());
		imguiApp::GetInstance().DrawCameraKeyframeEditorWindow();
	}

	//ステージ編集中は壁ゾーンの線枠と編集用のImGuiウィンドウを出す
	if (System::GetInstance().GetBattleMgr()->GetStageEditMode())
	{
		m_wallZoneEditor->DrawZones(*m_stage);
		m_wallZoneEditor->DrawWindow(*m_stage, m_player.get(),
			m_cameraManager->GetPhotoCameraPos(),
			m_cameraManager->GetPhotoCameraTarget());
	}

	//スクリーンショットが要求されていたら、全描画完了後に保存する
	if (m_requestScreenshot)
	{
		SaveDrawScreenToPNG(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), kScreenshotPath);
		m_requestScreenshot = false;
	}
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
