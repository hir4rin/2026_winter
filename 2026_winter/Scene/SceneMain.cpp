#include "SceneMain.h"
#include "Scene/SceneController.h"
#include "DxLib.h"
#include <cmath>
#include <cstring>
#include <unordered_map>
#include "Character/Player/Player.h"
#include "Character/Enemy/Swordman/EnemySwordman.h"
#include "Character/Enemy/EnemyManager.h"
#include "Camera/Camera.h"
#include "Camera/CameraManager.h"
#include "Camera/LockOnManager.h"
#include "DataLoader/DataManager.h"
#include "Collision/CollisionManager.h"
#include "Render/EffectManager.h"
#include "Render/BloodDecalManager.h"
#include "Render/ModelShaderManager.h"
#include "Render/SwordLook.h"
#include "Stage/Stage.h"
#include "Stage/SkyBox.h"
#include "Stage/TunnelEnding.h"
#include "Stage/TutorialFinale.h"
#include "Main/Input.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Battle/PhaseManager.h"
#include "Battle/CheckpointManager.h"
#include "Main/Game.h"
#include "Main/imguiApp.h"
#include "Main/FrameProfiler.h"
#include "Scene/PauseScene.h"
#include "Scene/TrainingMenuScene.h"
#include "Battle/TrainingManager.h"
#include "Scene/ResultScene.h"
#include "Scene/TutorialScene.h"
#include "Scene/GameOverScene.h"
#include "Battle/ResultRecorder.h"
#include "Stage/WallZone/WallZoneEditor.h"
#include "Stage/Phase/PhaseAreaEditor.h"
#include "Stage/Phase/CheckpointEditor.h"
#include "UI/UIManager.h"
#include "Camera/CameraState/PlayerStartMovieCamera/PlayerStartMovieCutCamera.h"
#include "Camera/CameraState/PlayerStartMovieCamera/PlayerStartMovieLandCamera.h"
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

	//スキルエフェクト確認用
	constexpr float kDebugSkillEffectScale = 20.0f;//分身モデルは1/20で作っているので20倍して元の大きさに戻す
	constexpr float kDebugSkillEffectDistance = 300.0f;//プレイヤーの正面のどこを中心にするか
	constexpr int kChargeHoldLabelFrame = 120;//F11で切り替えた案の名前を出すフレーム
}

SceneMain::SceneMain(SceneController& controller, StageType stageType, std::optional<RespawnInfo> respawn,
	std::optional<TrainingManager::Settings> trainingSettings) :
	Scene(controller),
	m_frameCount(0),
	m_fadeFrame(0),
	m_stageType(stageType),
	m_respawn(respawn),
	m_trainingSettings(trainingSettings)
{	m_updateFunc = static_cast<UpdateFunc_t>(&SceneMain::NormalUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&SceneMain::NormalDraw);
	Init();
	//トレーニングを作り直したときは暗いところから明ける
	if (m_trainingSettings)
	{
		m_updateFunc = static_cast<UpdateFunc_t>(&SceneMain::FadeInUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&SceneMain::FadeInDraw);
	}
}

SceneMain::~SceneMain()
{
	//プレイヤー・ステージのコライダーをマネージャーから外してからモデルを解放する
	CollisionManager::GetInstance().Terminate();
	//ステージのモデルを消す前に血の準備を片付ける
	if (auto bloodMgr = m_battleManager->GetBloodDecalMgr().lock()) bloodMgr->Terminate();
	m_player.reset();
	m_stage.reset();
	//シェーダーは毎回コンパイルすると重いので消さない(Application::Terminate で消す)
	ModelShaderManager::GetInstance().ResetSceneState();
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
	//DxLibの標準ライトは他のライトと重なって明るくなりすぎる(全体が白っぽくなる)ので無効化
	//標準ライトの環境光(0.33)の分は TempTopLight の環境光に足してある
	SetLightEnable(FALSE);
	m_lightHandle = CreateDirLightHandle(VNorm(kLightDir));
	SetLightDifColorHandle(m_lightHandle, GetColorF(kLightDifColorValue, kLightDifColorValue, kLightDifColorValue, 1.0f));
	SetLightSpcColorHandle(m_lightHandle, GetColorF(kLightSpcColorValue, kLightSpcColorValue, kLightSpcColorValue, 1.0f));
	SetLightAmbColorHandle(m_lightHandle, GetColorF(kLightAmbColorValue, kLightAmbColorValue, kLightAmbColorValue, 1.0f));
	m_tempTopLight.Init();//[TempTopLight]

	//CSVデータ(アニメーション名・コンボ)の読み込み
	DataManager::GetInstance().LoadAll();
	Input::GetInstance().Init();

	CollisionManager::GetInstance().Init();
	//モデル用のシェーダー(頂点タイプごとの頂点シェーダー・敵を黒くするピクセルシェーダー)
	ModelShaderManager::GetInstance().Init();

	//このステージのモデル・敵を読む(前に入ったステージと同じなら何もしない)
	System::GetInstance().LoadStage(m_stageType);

	//エネミーマネージャー
	m_enemyManager = std::make_shared<EnemyManager>();

	//ステージの生成//当たり判定の初期化のみ行う(モデル・CSVの読み込みなどのデータ部分は別途対応する)
	m_stage = std::make_shared<Stage>();
	m_stage->Init();
	const StageInfo& stageInfo = StageData::GetInfo(m_stageType);
	m_stage->GameInit(stageInfo);
	if (stageInfo.skyDir != nullptr)
	{
		m_skyBox = std::make_unique<SkyBox>();
		m_skyBox->Init(stageInfo.skyDir);
	}
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

	//チェックポイント(無ければ空。前のステージのが残らないように毎回読む)
	DataManager::GetInstance().LoadCheckpoints(stageInfo.enemySpawnNumber);
	//リスポーンならチェックポイントに置く
	const int respawnIndex = m_respawn ? m_respawn->checkpointIndex : -1;
	const auto& checkpoints = DataManager::GetInstance().GetCheckpoints();
	const CheckpointData* respawnCp = (respawnIndex >= 0 && respawnIndex < static_cast<int>(checkpoints.size())) ? &checkpoints[respawnIndex] : nullptr;
	if (respawnCp)
	{
		m_player->SetPos(respawnCp->position);
		m_player->SetFacing(respawnCp->rotYDeg * DX_PI_F / 180.0f);
	}

	//敵の生成
	m_enemyManager->SetPlayer(m_player);
	//Unityで書き出した敵配置CSVを読み込んで、最初のフェーズの敵を出す
	DataManager::GetInstance().LoadEnemySpawnData(stageInfo.enemySpawnNumber);
	const auto& spawnData = DataManager::GetInstance().GetEnemySpawnData();
	m_phaseManager = std::make_shared<PhaseManager>();
	if (!spawnData.empty())
	{
		DataManager::GetInstance().LoadPhaseFlow(stageInfo.enemySpawnNumber);
		DataManager::GetInstance().LoadPhaseAreas(stageInfo.enemySpawnNumber);
		const int startPhase = respawnCp ? respawnCp->phase : 0;
		m_phaseManager->Init(m_enemyManager, m_player, startPhase, m_respawn ? m_respawn->killCount : 0);
		if (m_respawn) m_stage->SnapGates(m_phaseManager->GetCurrentPhase());
	}
	else if (m_stageType == StageType::Training)
	{
		//トレーニングは設定メニューの値で敵を出す
		m_training = std::make_shared<TrainingManager>();
		m_training->Init(m_enemyManager, m_player, m_trainingSettings.value_or(TrainingManager::Settings{}));
	}
	else
	{
		//配置データが無いステージは、今まで通りテスト用の敵を1体出す
		auto enemy = std::make_shared<EnemySwordman>(m_player, kEnemyStartPos);
		enemy->Init();
		m_enemyManager->AddEnemy(enemy);//追加
	}
	m_phaseAreaEditor = std::make_unique<PhaseAreaEditor>();
	m_phaseAreaEditor->SetStageNumber(stageInfo.enemySpawnNumber);
	m_checkpointManager = std::make_shared<CheckpointManager>();
	m_checkpointManager->Init(m_player, respawnCp ? respawnIndex : -1);
	m_checkpointEditor = std::make_unique<CheckpointEditor>();
	m_checkpointEditor->SetStageNumber(stageInfo.enemySpawnNumber);


	m_battleManager = std::make_shared<BattleManager>();
	m_battleManager->Init();
	System::GetInstance().SetBattleMgr(m_battleManager);
	if (auto bloodMgr = m_battleManager->GetBloodDecalMgr().lock()) bloodMgr->Init(m_stage->GetStageModelHandle(), m_stage->GetStageViewHandle());
	//記録は死ぬ前の続きから
	if (m_respawn)
	{
		if (auto recorder = m_battleManager->GetResultRecorder().lock()) *recorder = m_respawn->recorder;
	}

	//プレイヤー追従カメラ
	//リスポーンはチェックポイントの向きの後ろから始める
	const float cameraAngleH = respawnCp ? respawnCp->rotYDeg * DX_PI_F / 180.0f : 0.0f;
	m_cameraManager->Init(m_player, m_stage, cameraAngleH);
	m_cameraManager->SetCheckpointManager(m_checkpointManager);
	if (m_stageType == StageType::Cyber && !m_respawn) StartPlayerStartMovie(PlayerStartMovieFrame::Kind::Roof);
	if (m_stageType == StageType::Tutorial)
	{
		if (!m_respawn) StartPlayerStartMovie(PlayerStartMovieFrame::Kind::Fall);
		m_tutorialFinale = std::make_unique<TutorialFinale>(m_stage, m_player, m_cameraManager);
	}

	//インゲーム用のUIを生成する(プレイヤーなどを参照するUIがあるので最後に行う)
	m_uiManager = std::make_unique<UIManager>();
	m_uiManager->InGameInit(m_cameraManager, m_player, m_enemyManager);
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

	//トレーニングはStartで設定メニューを積む(ポーズの代わり)
	if (m_training && input.IsTriggered("Start") && !isImGuiTyping)
	{
		m_controller.PushScene(std::make_shared<TrainingMenuScene>(m_controller, m_training));
		return;
	}

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
		PauseUI::Record record;
		record.stageName = StageData::GetInfo(m_stageType).name;
		if (auto recorder = battleMgr->GetResultRecorder().lock())
		{
			record.elapsedFrame = recorder->GetClearFrame();
		}
		record.killCount = m_enemyManager->GetTotalKillCount();
		m_controller.PushScene(std::make_shared<PauseScene>(m_controller, record));
		return;
	}

	//タイムスケールを0.1にする
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

	FrameProfiler::GetInstance().Mark("Upd:Before");
	m_player->Update(*m_camera);
	FrameProfiler::GetInstance().Mark("Upd:Player");
	m_enemyManager->Update();
	if (m_training) m_training->Update();
	m_phaseManager->Update();
	m_checkpointManager->Update();
	m_stage->UpdateGates(m_phaseManager->GetCurrentPhase());//押し戻しの前に開け閉めする	FrameProfiler::GetInstance().Mark("Upd:Enemy");
	m_stage->Update();
	CollisionManager::GetInstance().Update();
	FrameProfiler::GetInstance().Mark("Upd:Collision");
	System::GetInstance().Update();
	m_battleManager->Update();//Systemでタイムスケールを戻した後に更新する

	//エンディング(最後のフェーズの遷移表の event が Ending)
	//F7 か起動の引数 -ending で、敵を消してすぐ始める(確認用)
	const bool isF7Pressed = CheckHitKey(KEY_INPUT_F7) != 0;
	const bool isDebugEnding = (isF7Pressed && !m_wasF7Pressed) || (m_frameCount == 2 && strstr(GetCommandLineA(), "-ending"));
	m_wasF7Pressed = isF7Pressed;
	if (isDebugEnding && !m_ending)
	{
		m_enemyManager->DebugClearEnemies();
		m_phaseManager->DebugFinish();
	}
	if ((m_phaseManager->TakeEndingRequest() || isDebugEnding) && !m_ending) StartEnding();
	if (m_ending) m_ending->Update();
	if (m_tutorialFinale) m_tutorialFinale->Update();
	//押し戻しの後のプレイヤーの位置で、近い明かりを点ける
	m_stage->UpdateLights(m_player->GetRigidBody().GetPos());

	m_cameraManager->Update();

	m_uiManager->Update();

	//血殺中はステージの色を変える
	m_stageColorGrade.Update(battleMgr->GetIsUltimating());

//#ifdef _DEBUG
	//F4キーでボスの一つ手前のフェーズへ飛ぶ
	const bool isF4Pressed = CheckHitKey(KEY_INPUT_F4) != 0;
	if (isF4Pressed && !m_wasF4Pressed)
	{
		m_phaseManager->DebugSkipToBossPrevPhase();
	}
	m_wasF4Pressed = isF4Pressed;
//#endif

	//F5/F6/F9キーでスキルエフェクトを再生する(エフェクト確認用)
	const bool isF5Pressed = CheckHitKey(KEY_INPUT_F5) != 0;
	const bool isF6Pressed = CheckHitKey(KEY_INPUT_F6) != 0;
	const bool isF9Pressed = CheckHitKey(KEY_INPUT_F9) != 0;
	if (isF5Pressed && !m_wasF5Pressed)
	{
		PlayDebugSkillEffect(System::GetInstance().GetHandle(AsyncData::DebugGhostDashEffect), kDebugSkillEffectDistance);
	}
	if (isF6Pressed && !m_wasF6Pressed)
	{
		PlayDebugSkillEffect(System::GetInstance().GetHandle(AsyncData::DebugGhostDash3DEffect), kDebugSkillEffectDistance);
	}
	//地上弱スキル//押すたびに1→2→3段目
	//1,2は強スキルと同じく敵の位置(正面)に出す//3はプレイヤーの足元から、ゲームでは逆を向くので180度ずらす
	if (isF9Pressed && !m_wasF9Pressed)
	{
		const auto key = static_cast<AsyncData>(static_cast<int>(AsyncData::DebugGleamLight1Effect) + m_debugGleamStep);
		const bool isStep3 = m_debugGleamStep == 2;
		PlayDebugSkillEffect(System::GetInstance().GetHandle(key), isStep3 ? 0.0f : kDebugSkillEffectDistance, isStep3 ? DX_PI_F : 0.0f);
		m_debugGleamStep = (m_debugGleamStep + 1) % 3;
	}
	m_wasF5Pressed = isF5Pressed;
	m_wasF6Pressed = isF6Pressed;
	m_wasF9Pressed = isF9Pressed;

	//F11で溜め中のエフェクトの案を切り替える(確認用)
	const bool isF11Pressed = CheckHitKey(KEY_INPUT_F11) != 0;
	if (isF11Pressed && !m_wasF11Pressed)
	{
		System::GetInstance().CycleChargeHoldType();
		m_chargeHoldLabelFrame = kChargeHoldLabelFrame;
	}
	m_wasF11Pressed = isF11Pressed;
	if (m_chargeHoldLabelFrame > 0) --m_chargeHoldLabelFrame;

	//再生中のエフェクトを、このフレームのタイムスケールに合わせて進める(自分の速さを持つエフェクトの設定し直しもここで行う)
	EffectManager::GetInstance().Update();

	//プレイヤーが死んだらゲームオーバーシーンを積む
	if (m_player->GetIsDead())
	{
		m_controller.PushScene(std::make_shared<GameOverScene>(m_controller, m_stageType, MakeRespawnInfo()));
		return;
	}

	//バトルが終わったらフェードアウトしてリザルトへ
	if (m_battleManager->GetIsBattleEnd())
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&SceneMain::FadeOutUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&SceneMain::FadeOutDraw);
	}
	//チュートリアルを開く
	if (m_phaseManager->IsTutorialReady() && CanOpenTutorial())
	{
		m_controller.PushScene(std::make_shared<TutorialScene>(m_controller, m_phaseManager->TakeTutorial()));
	}


}

void SceneMain::PlayDebugSkillEffect(int effectHandle, float forwardDistance, float rotOffset)
{
	const Vector3 playerPos = m_player->GetRigidBody().GetPos();
	const Vector3 forward = m_player->GetTargetVec();
	const Vector3 center = playerPos + forward * forwardDistance;

	const int playingHandle = PlayEffekseer3DEffect(effectHandle);
	SetPosPlayingEffekseer3DEffect(playingHandle, center.x, center.y, center.z);
	//プレイヤーと同じ向きにする
	SetRotationPlayingEffekseer3DEffect(playingHandle, 0.0f, atan2f(forward.x, forward.z) + rotOffset, 0.0f);
	SetScalePlayingEffekseer3DEffect(playingHandle, kDebugSkillEffectScale, kDebugSkillEffectScale, kDebugSkillEffectScale);
}

bool SceneMain::CanOpenTutorial() const
{
	if (m_battleManager->GetIsUltimating())return false;//血殺中
	if (m_battleManager->GetIsEventPlaying())return false;//イベント中
	if (m_player->GetIsAssasinating())return false;//暗殺中
	if (m_player->GetGuardBreakChanceState())return false;//ガードブレイクのチャンス中
	return true;
}

void SceneMain::StartEnding()
{
	if (!m_stage->HasTunnel())
	{
		m_battleManager->SetIsBattleEnd(true);
		return;
	}
	m_ending = std::make_unique<TunnelEnding>(m_stage, m_player, m_cameraManager);
}

RespawnInfo SceneMain::MakeRespawnInfo() const
{
	RespawnInfo info;
	if (auto recorder = m_battleManager->GetResultRecorder().lock()) info.recorder = *recorder;
	info.recorder.OnPlayerDead();

	const CheckpointData* cp = m_checkpointManager->GetLastCheckpoint();
	if (!cp) return info;//最初から(撃破数も0)
	info.checkpointIndex = m_checkpointManager->GetLastIndex();
	if (cp->phase > 0)
	{
		//フェーズが始まっていればその時の数、まだなら今の数(そのフェーズの敵は出し直す)
		const int startKill = m_phaseManager->GetKillCountAtPhaseStart(cp->phase);
		info.killCount = startKill >= 0 ? startKill : m_enemyManager->GetTotalKillCount();
	}
	return info;
}

void SceneMain::StartPlayerStartMovie(PlayerStartMovieFrame::Kind kind)
{
	PlayerStartMovieFrame frame;
	frame.kind = kind;
	frame.origin = m_player->GetRigidBody().GetPos();
	Vector3 front = m_player->GetTargetVec();
	front.y = 0.0f;
	if (front.Magnitude() > 0.0f) frame.front = front.Normalize();

	m_player->StartStartMovie(frame);
	m_cameraManager->ChangeState(std::make_shared<PlayerStartMovieCutCamera>(m_cameraManager, frame));
}

void SceneMain::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		const StageInfo& info = StageData::GetInfo(m_stageType);
		const int spawnNum = static_cast<int>(DataManager::GetInstance().GetEnemySpawnData().size());
		const int enemyNum = spawnNum > 0 ? spawnNum : 1;
		ResultData result;
		if (auto recorder = m_battleManager->GetResultRecorder().lock())
		{
			result = recorder->MakeResult(static_cast<int>(m_stageType), info.targetClearSec, m_enemyManager->GetTotalKillCount(), enemyNum);
		}
		//このシーンはここで破棄されるので、呼んだ後は何もしない
		m_controller.ResetScene<ResultScene>(m_stageType, result);
		return;
	}
}

void SceneMain::FadeInDraw()
{
	NormalDraw();
	DrawFade();
}

void SceneMain::NormalDraw()
{
	if (m_skyBox) m_skyBox->Draw();
	m_stageColorGrade.DrawBackground();

	DrawGrid();
	FrameProfiler::GetInstance().Mark("Draw:Bg");

	//ステージを先に描く(明かりの光の重ね描きは深さを少し手前にずらしているので、キャラを後に描いて足元へのはみ出しを上書きする)
	m_stageColorGrade.Begin();
	m_stage->Draw();
	if (auto bloodMgr = m_battleManager->GetBloodDecalMgr().lock()) bloodMgr->Draw();//床の血の跡
	m_stageColorGrade.End();
	FrameProfiler::GetInstance().Mark("Draw:Stage");
	//2DのUIは深度を書かないので、後から描いた3Dに上書きされる
	//敵・プレイヤーより後ろに出したいUI(血殺の文字など)はここで描く
	m_uiManager->DrawBack();
	//血殺中は敵を真っ黒にする
	ModelShaderManager::GetInstance().SetBlackMode(System::GetInstance().GetBattleMgr()->GetIsUltimating());
	m_enemyManager->Draw();
	ModelShaderManager::GetInstance().SetBlackMode(false);
	FrameProfiler::GetInstance().Mark("Draw:Enemy");

	m_player->Draw();
	FrameProfiler::GetInstance().Mark("Draw:Player");

	//DxLibのカメラ設定をEffekseerに反映してからエフェクトを描画する
	Effekseer_Sync3DSetting();
	DrawEffekseer3D();
	FrameProfiler::GetInstance().Mark("Draw:Effect");
	//[TEMP_TRAIL] 確認用
	{
		extern int g_trailCapture;
		if (g_trailCapture > 0 && g_trailCapture <= 70)
		{
			char path[64];
			sprintf_s(path, "Log/trail_%03d.png", g_trailCapture++);
			int w = 0, h = 0;
			GetDrawScreenSize(&w, &h);
			SaveDrawScreenToPNG(0, 0, w, h, path);
		}
	}

	
	m_uiManager->Draw();

	//F11で切り替えた溜め中のエフェクトの案の名前
	if (m_chargeHoldLabelFrame > 0)
	{
		DrawFormatString(20, 200, GetColor(255, 230, 120), "溜めエフェクト[F11]: %s", System::GetInstance().GetChargeHoldName());
	}
	FrameProfiler::GetInstance().Mark("Draw:UI");
	//カクつきの切り分け(F2。Release でも出す)
	if (imguiApp::GetInstance().IsDebugUIVisible())m_cameraManager->DrawDebugWindow();
#ifdef _DEBUG
	CollisionManager::GetInstance().DebugDraw();
	//敵のデバッグ表示(頭の上の文字)//2Dの文字は後から描く3Dに上書きされるので、UIの後に描く
	m_enemyManager->DebugDraw();
	//ボスのデバッグウィンドウ(F2で表示/非表示)
	if (imguiApp::GetInstance().IsDebugUIVisible())m_enemyManager->DrawDebugWindows();
	//ステージの明かりの調整(F2で表示/非表示。明かりの無いステージでは出ない)
	if (imguiApp::GetInstance().IsDebugUIVisible())m_stage->DrawLightDebugWindow();
	//キャラの見た目(光の色・ふちの光など)の調整
	if (imguiApp::GetInstance().IsDebugUIVisible())CharacterLook::DrawDebugWindow();
	//プレイヤーの刀の見た目(映り込み・刃文・刃先の光)
	if (imguiApp::GetInstance().IsDebugUIVisible())SwordLook::DrawDebugWindow();
	m_phaseManager->DebugDraw();
	m_player->GetLockOnManager()->DebugDraw();
	//ステージ編集中はエディタ側で(選択中を白にして)描く
	if (!System::GetInstance().GetBattleMgr()->GetStageEditMode())m_phaseManager->DrawAreas();
	m_stage->DrawGateCols();//扉の当たり判定(赤=閉、緑=開)
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
		imguiApp::GetInstance().DrawCameraKeyframeEditorWindow(m_player->GetRigidBody().GetPos());
	}

	//ステージ編集中は壁ゾーンの線枠と編集用のImGuiウィンドウを出す
	if (System::GetInstance().GetBattleMgr()->GetStageEditMode())
	{
		m_wallZoneEditor->DrawZones(*m_stage);
		m_wallZoneEditor->DrawWindow(*m_stage, m_player.get(),
			m_cameraManager->GetPhotoCameraPos(),
			m_cameraManager->GetPhotoCameraTarget());
		m_phaseAreaEditor->DrawAreas(*m_phaseManager);
		m_phaseAreaEditor->DrawWindow(*m_phaseManager, *m_stage, m_player.get(),
			m_cameraManager->GetPhotoCameraTarget());
		m_checkpointEditor->DrawCheckpoints(*m_checkpointManager);
		m_checkpointEditor->DrawWindow(*m_checkpointManager, *m_stage, m_player.get(),
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
