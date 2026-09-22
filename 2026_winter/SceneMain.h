#pragma once
#include "DxLib.h"
#include <memory>

class Player;
class CameraManager;
class Camera;
class Stage;

class SceneMain
{
public:
	SceneMain();
	~SceneMain();

	void Init();
	void Update();
	void Draw();
private:
	void DrawGrid();

private:
	int m_frameCount;

	int m_lightHandle = -1;

	std::shared_ptr<Player> m_player;
	std::shared_ptr<CameraManager> m_cameraManager;
	std::unique_ptr<Camera> m_camera;//Player::Updateに渡すだけ(実際のカメラはCameraManagerが制御する)
	std::shared_ptr<Stage> m_stage;
};
