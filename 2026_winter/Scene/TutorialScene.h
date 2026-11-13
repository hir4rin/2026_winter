#pragma once
#include "Scene.h"
#include <string>
#include <vector>

/// <summary>
/// チュートリアルシーン(下のシーンに重ねて出す。A/左右でページ送り、Bで閉じる)
/// </summary>
class TutorialScene :
	public Scene
{
private:
	struct Page
	{
		int handle = -1;
		std::string title;
		std::string text;
	};
public:
	//id: Tutorial.csv の id
	TutorialScene(SceneController& controller, const std::string& id);
	virtual ~TutorialScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;
private:
	void StartFadeOut();
	//0:透明 1:表示
	void DrawContent(float alpha);
private:
	std::vector<Page> m_pages;
	int m_pageIndex = 0;

	int m_fadeFrame = 0;
};
