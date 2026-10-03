#pragma once
#include "Scene.h"

/// <summary>
/// ステージセレクトシーン
/// 上下でステージを選んでAで決定すると、選んだステージでSceneMainを始める
/// </summary>
class StageSelectScene :
	public Scene
{
public:
	StageSelectScene(SceneController& controller);
	virtual ~StageSelectScene();

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

	//選択中のステージ(StageTypeのint値)
	//シーンを作り直しても前回選んだステージにカーソルを合わせておく
	static int s_cursor;
	int m_fadeFrame = 0;//フェードの経過フレーム
};
