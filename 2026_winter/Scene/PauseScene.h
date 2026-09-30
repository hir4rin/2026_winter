#pragma once
#include "Scene.h"

/// <summary>
/// ポーズシーン
/// </summary>
class PauseScene :
	public Scene
{
public:
	PauseScene(SceneController& controller);
	virtual ~PauseScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;
};

