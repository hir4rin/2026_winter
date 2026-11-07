#pragma once
#include "UIBase.h"

//トドメのムービーの画面効果//上下の黒帯・白いフラッシュ・斬撃の線
class FinisherUI : public UIBase
{
public:
	void Init()override;
	void Update()override;
	void Draw()override;

private:
	float m_barRate = 0.0f;//黒帯の出具合
	float m_whiteRate = 0.0f;
	float m_slashRate = 0.0f;
};
