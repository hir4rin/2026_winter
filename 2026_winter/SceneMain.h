#pragma once
#include "DxLib.h"
#include <memory>
#include "Scene/Scene.h"
#include "Stage/StageInfo.h"

class Player;
class EnemySwordman;
class CameraManager;
class EnemyManager;
class BattleManager;
class Camera;
class Stage;
class WallZoneEditor;

class SceneMain : public Scene
{
public:
	SceneMain(SceneController& controller, StageType stageType);
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

	StageType m_stageType;//遊ぶステージ(StageSelectSceneで選んだもの)

	std::shared_ptr<Player> m_player;
	std::shared_ptr<EnemyManager> m_enemyManager;
	std::shared_ptr<CameraManager> m_cameraManager;
	std::shared_ptr<BattleManager> m_battleManager;
	std::unique_ptr<Camera> m_camera;//Player::Updateに渡すだけ(実際のカメラはCameraManagerが制御する)
	std::shared_ptr<Stage> m_stage;
	std::unique_ptr<WallZoneEditor> m_wallZoneEditor;//ステージ編集モードで壁ゾーンを設置・編集するImGuiウィンドウ

	bool m_requestScreenshot = false;//次のDrawの最後でスクリーンショットを保存するか
};
