#pragma once
#include "Scene.h"
#include <memory>

class CameraManager;
class TitleCamera;
class UIManager;
class TitleLogoUI;
class TitleFlashUI;
class TitlePressUI;
class TitlePlayer;
class TitleCity;

/// <summary>
/// タイトル画面。カメラとUIを持ち、決まったフレームで合図を出す。Aでステージセレクトへ
/// </summary>
class TitleScene :
	public Scene
{
public:
	TitleScene(SceneController& controller);
	virtual ~TitleScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

private:
	//演出の合図(斬る・揺らす・ボタンを出す)
	void UpdateDirection();
	//斬り終わった所まで飛ばす
	void SkipDirection();
	void DrawFade();

	std::shared_ptr<CameraManager> m_cameraManager;
	std::shared_ptr<TitleCamera> m_camera;
	std::unique_ptr<UIManager> m_uiManager;
	std::shared_ptr<TitleLogoUI> m_logo;
	std::shared_ptr<TitleFlashUI> m_flash;
	std::shared_ptr<TitlePressUI> m_press;
	std::unique_ptr<TitlePlayer> m_player;//右の後ろ姿
	std::unique_ptr<TitleCity> m_city;//背景の街(ステージ1)

	int m_frame = 0;//シーンが始まってからのフレーム
	int m_fadeFrame = 0;
};
