#pragma once
#include "UI/UIBase.h"

//タイトルの背景(暗い色と周辺減光)
class TitleBackUI : public UIBase
{
public:
	~TitleBackUI();
	void Init()override;
	void Update()override;
	void Draw()override;
	UILayer GetLayer()const override { return UILayer::Back; }

private:
	int m_vignetteHandle = -1;
};
