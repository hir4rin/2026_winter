#pragma once
#include "DxLib.h"
#include <memory>
#include "Scene/Scene.h"

class Player;
class CameraManager;
class Camera;
class Stage;

class SceneMain : public Scene
{
public:
	SceneMain(SceneController& controller);
	~SceneMain();

	void Update() override;
	void Draw() override;
private:
	void Init();

	//状態ごとの更新(m_updateFuncで切り替える)
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	//状態ごとの描画(m_drawFuncで切り替える)
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

	void DrawGrid();
	//フェード用の黒い板を描画する
	void DrawFade();

private:
	int m_frameCount;
	int m_fadeFrame;//フェードの経過フレーム

	int m_lightHandle = -1;

	std::shared_ptr<Player> m_player;
	std::shared_ptr<CameraManager> m_cameraManager;
	std::unique_ptr<Camera> m_camera;//Player::Updateに渡すだけ(実際のカメラはCameraManagerが制御する)
	std::shared_ptr<Stage> m_stage;
};
