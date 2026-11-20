#pragma once
#include "UIBase.h"

//PRESS A BUTTON(明滅)
class TitlePressUI : public UIBase
{
public:
	~TitlePressUI();
	void Init()override;
	void Update()override;
	void Draw()override;

	void Show();

private:
	int m_font = -1;
	int m_frame = 0;
};
