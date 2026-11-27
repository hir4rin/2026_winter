#pragma once
#include "Scene.h"
#include "UI/Menu/TrainingMenuUI.h"

/// <summary>
/// カメラ設定(ポーズの上に積む)
/// 変えた値はすぐ反映する
/// </summary>
class CameraOptionScene :
	public Scene
{
public:
	CameraOptionScene(SceneController& controller);
	virtual ~CameraOptionScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

private:
	//メニューの項目(並びは画面の順)
	enum class MenuItem
	{
		Sensitivity,//カメラ感度
		InvertX,//左右反転
		InvertY,//上下反転
		Back,//戻る
		Num
	};

	int m_cursor = 0;
	TrainingMenuUI m_ui;//値を変えるメニューの見た目はトレーニングと同じ
	int m_decideFrame = 0;
	//長押しで進める(↑↓←→の押している長さ)
	int m_holdUp = 0, m_holdDown = 0, m_holdLeft = 0, m_holdRight = 0;

	//押した瞬間か、長押しで繰り返すフレームなら true(isFirst: 押した瞬間か)
	static bool IsRepeat(const char* name, int& holdFrame, bool& isFirst);
	//値を d だけ変える
	void ChangeValue(int d);
	//値の文字・説明をUIに渡す
	void ApplyToUI();
};
