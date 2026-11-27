#pragma once
#include "Scene.h"
#include "UI/Menu/PauseUI.h"
#include <vector>

/// <summary>
/// ポーズシーン
/// </summary>
class PauseScene :
	public Scene
{
public:
	PauseScene(SceneController& controller, const PauseUI::Record& record);
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
		CameraOption,//カメラ設定
		PhotoMode,//フォトモード(カメラデバッグ)
		StageEdit,//ステージ編集(壁ゾーン)
		StageSelect,//ステージセレクトへ戻る
		Num
	};
	//L3+R3で開発用の項目(フォトモード・ステージ編集)を出す。ゲームを閉じるまで出たまま
	static bool s_isDebugItemOpen;
	std::vector<MenuItem> m_items;//今出している項目
	int m_cursor = 0;//選択中の項目(m_itemsの番号)
	PauseUI m_ui;
	MenuItem m_decideItem = MenuItem::ReturnGame;//決定の演出が終わったら実行する項目
	int m_decideFrame = 0;
	bool m_isChildOpen = false;//カメラ設定を上に積んでいる間は描かない

	//選択した項目を実行する
	void Decide(MenuItem item);
	//出す項目を作り直す(カーソルは同じ項目に残す)
	void RebuildItems();
	//説明をカーソルの項目に合わせる
	void ApplyCursor();
};
