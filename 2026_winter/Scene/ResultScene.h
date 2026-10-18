#pragma once
#include "Scene.h"

/// <summary>
/// リザルトシーン(仮)
/// Aでステージセレクトに戻る
/// </summary>
class ResultScene :
	public Scene
{
public:
	ResultScene(SceneController& controller);
	virtual ~ResultScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

private:
	//フェード用の黒い板を描画する
	void DrawFade();

	int m_fadeFrame = 0;//フェードの経過フレーム
};
