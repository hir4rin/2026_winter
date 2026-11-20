#include "TitleScene.h"
#include "SceneController.h"
#include "StageSelectScene.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Camera/CameraManager.h"
#include "Camera/CameraState/TitleCamera.h"
#include "UI/UIManager.h"
#include "UI/TitleLogoUI.h"
#include "UI/TitleFlashUI.h"
#include "UI/TitlePressUI.h"
#include "Character/Player/TitlePlayer.h"
#include "Stage/TitleCity.h"

namespace
{
	constexpr int kFadeFrame = 30;

	//演出の合図(シーンの始まりから)
	constexpr int kSlashFrame = 85;//斬り口の光が走り始める
	constexpr int kImpactFrame = kSlashFrame + 4;//上がずれる・揺れ・白い光
	constexpr int kPressFrame = kImpactFrame + 51;

	constexpr float kShakePower = 9.0f;//タイトルカメラでは設計解像度のピクセル
	constexpr float kShakeTime = 20.0f;

	constexpr float kCameraNear = 100.0f;
	constexpr float kCameraFar = 8000.0f;
}

TitleScene::TitleScene(SceneController& controller) :Scene(controller)
{
	//最初のシーンなので、ここで入力を使えるようにしておく
	Input::GetInstance().Init();
	System::GetInstance().SetTimeScale(1.0f);

	m_updateFunc = static_cast<UpdateFunc_t>(&TitleScene::FadeInUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&TitleScene::FadeInDraw);

	m_cameraManager = std::make_shared<CameraManager>();
	m_camera = std::make_shared<TitleCamera>(m_cameraManager);
	m_cameraManager->InitState(m_camera);
	SetCameraNearFar(kCameraNear, kCameraFar);

	m_city = std::make_unique<TitleCity>();
	m_city->Init();
	m_player = std::make_unique<TitlePlayer>();
	m_player->Init();

	m_uiManager = std::make_unique<UIManager>();
	m_uiManager->TitleInit();
	//ロゴ「忍」と斬った瞬間の閃光のエフェクト
	m_logo = std::make_shared<TitleLogoUI>();
	m_uiManager->Add(m_logo);
	//画面の白い光
	m_flash = std::make_shared<TitleFlashUI>();
	m_uiManager->Add(m_flash);
	//PRESS A BUTTON
	m_press = std::make_shared<TitlePressUI>();
	m_uiManager->Add(m_press);
	//登録したUIのハンドルを生成する
	m_uiManager->InitAll();
}

TitleScene::~TitleScene()
{
}

void TitleScene::Update()
{
	m_frame++;
	UpdateDirection();
	(this->*m_updateFunc)();
	m_cameraManager->Update();
	m_player->Update();
	m_uiManager->Update();
}

void TitleScene::UpdateDirection()
{
	if (m_frame == kSlashFrame)m_logo->Slash();
	if (m_frame == kImpactFrame)
	{
		m_cameraManager->StartCameraShake(kShakePower, kShakeTime);
		m_flash->Start();
		m_player->OnImpact();
	}
	if (m_frame == kPressFrame)m_press->Show();
}

void TitleScene::SkipDirection()
{
	m_frame = kPressFrame;
	m_camera->SkipIntro();
	m_logo->Skip();
	m_press->Show();
	m_player->Skip();
}

void TitleScene::FadeInUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&TitleScene::NormalUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&TitleScene::NormalDraw);
	}
}

void TitleScene::NormalUpdate()
{
	auto& input = Input::GetInstance();
	input.Update();
	if (!input.IsTriggered("A"))return;

	if (m_frame < kPressFrame)
	{
		SkipDirection();
		return;
	}
	m_fadeFrame = 0;
	m_updateFunc = static_cast<UpdateFunc_t>(&TitleScene::FadeOutUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&TitleScene::FadeOutDraw);
}

void TitleScene::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		//このシーンはここで破棄されるので、呼んだ後は何もしない
		m_controller.ResetScene<StageSelectScene>();
	}
}

void TitleScene::Draw()
{
	(this->*m_drawFunc)();
}

void TitleScene::FadeInDraw()
{
	NormalDraw();
	DrawFade();
}

void TitleScene::NormalDraw()
{
	m_city->Draw();
	m_uiManager->DrawBack();
	m_player->Draw();
	m_uiManager->Draw();
}

void TitleScene::FadeOutDraw()
{
	NormalDraw();
	DrawFade();
}

void TitleScene::DrawFade()
{
	//フェードイン中は黒→透明、フェードアウト中は透明→黒
	float rate = static_cast<float>(m_fadeFrame) / static_cast<float>(kFadeFrame);
	if (m_updateFunc == static_cast<UpdateFunc_t>(&TitleScene::FadeInUpdate))
	{
		rate = 1.0f - rate;
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
