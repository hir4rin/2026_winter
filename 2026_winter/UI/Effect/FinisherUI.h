#pragma once
#include "UI/UIBase.h"

//トドメのムービーの画面効果//白いフラッシュ
class FinisherUI : public UIBase
{
public:
	void Init()override;
	void Update()override;
	void Draw()override;

private:
	float m_whiteRate = 0.0f;
};
