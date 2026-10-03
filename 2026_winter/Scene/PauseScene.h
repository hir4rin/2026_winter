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

private:
	//ポーズメニューの項目
	enum class MenuItem
	{
		ReturnGame,//ゲームに戻る
		PhotoMode,//フォトモード(カメラデバッグ)
		StageEdit,//ステージ編集(壁ゾーン)
		StageSelect,//ステージセレクトへ戻る
		Num
	};
	int m_cursor = 0;//選択中の項目(MenuItemのint値)

	//選択した項目を実行する
	void Decide();
};

