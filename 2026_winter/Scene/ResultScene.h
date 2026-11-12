#pragma once
#include "Scene.h"
#include "Stage/StageInfo.h"
#include "Battle/ResultRecorder.h"
#include "UI/ResultUI.h"

/// <summary>
/// リザルトシーン
/// 演出中はAで飛ばす。終わったらAでステージセレクト、Yでリトライ
/// </summary>
class ResultScene :
	public Scene
{
public:
	//isSaveRecord:自己ベストを保存するか(デバッグ起動はfalse)
	ResultScene(SceneController& controller, StageType stageType, const ResultData& data, bool isSaveRecord = true);
	virtual ~ResultScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

	//デバッグ起動(-result)用の仮の結果
	static ResultData MakeDebugData();

private:
	//フェード用の黒い板を描画する
	void DrawFade();
	void LoadingUpdate();
	void LoadingDraw();

	//自己ベスト(Save/BestScore.txt。1行に「ステージ番号 スコア」)
	static int LoadBestScore(int stage);
	static void SaveBestScore(int stage, int score);

	StageType m_stageType;
	ResultUI m_ui;
	bool m_isRetry = false;//フェードアウトの後にもう一度遊ぶか
	int m_fadeFrame = 0;//フェードの経過フレーム
};
