#pragma once
#include "UIBase.h"

//斬った瞬間の画面の白い光
class TitleFlashUI : public UIBase
{
public:
	void Init()override;
	void Update()override;
	void Draw()override;

	void Start();

private:
	int m_frame = 0;
};
