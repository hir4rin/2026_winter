#pragma once
#include "MenuUI.h"

//ポーズ画面(左に一覧と説明、右上に任務の記録。右はゲーム画面を少し暗くして見せる)
class PauseUI :
	public MenuUI
{
public:
	//右上に出す記録
	struct Record
	{
		std::string stageName;
		int elapsedFrame = 0;//実フレーム
		int killCount = 0;
	};

	~PauseUI() override;

	void Init() override;
	void Update() override;
	void Draw() override;

	void SetRecord(const Record& record) { m_record = record; }

private:
	void DrawShade();
	void DrawDescription();
	void DrawRecord();
	void DrawPauseGuide();
	//開いてから delay フレーム後に横から出てくる(dir: 1で左から、-1で右から、0でその場)
	void ApplyOpen(int delay, float dir);

	Record m_record;
	int m_openFrame = 0;
	int m_buttonBHandle = -1;
};
