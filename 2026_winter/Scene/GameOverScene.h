#pragma once
#include "Scene.h"
#include "Stage/StageInfo.h"
#include "Battle/RespawnInfo.h"

/// <summary>
/// ゲームオーバーシーン(SceneMainに重ねて出す。Aで最後のチェックポイントから)
/// </summary>
class GameOverScene :
	public Scene
{
public:
	GameOverScene(SceneController& controller, StageType stageType, const RespawnInfo& respawn);
	virtual ~GameOverScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

private:
	void DrawContent();
	void DrawShi();
	void DrawGuide();

	int m_shiHandle = -1;
	int m_shiDripHandle = -1;
	int m_vignetteHandle = -1;
	int m_buttonAHandle = -1;
	int m_fontHandle = -1;

	StageType m_stageType;
	RespawnInfo m_respawn;

	int m_frame = 0;//シーンが始まってからのフレーム
	int m_fadeFrame = 0;
};
