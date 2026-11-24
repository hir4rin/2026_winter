#pragma once
#include "UI/UIBase.h"

//トドメのムービーの画面効果//白いフラッシュ、ボスが切れる線に沿った血殺の線
class FinisherUI : public UIBase
{
public:
	~FinisherUI()override;
	void Init()override;
	void Update()override;
	void Draw()override;

private:
	//ボスが切れる線を画面の上から下まで描き、画面の右端に縦の線の束を出す//ボスより手前
	void DrawCut();

	float m_whiteRate = 0.0f;

	int m_cutHandle = -1;//切れる線(細い筆の線の束)
	int m_edgeHandle = -1;//画面の右端の縦の線の束
	bool m_isCutFlip = false;//切れる線を上下反転するか(出すたびに変える)
	float m_cutAlphaRate = 0.0f;//血殺の間1、終わったら下げる
	bool m_wasCutLine = false;
};
