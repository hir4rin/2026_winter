#pragma once
#include "UI/UIBase.h"
#include <vector>

//右から左へ流れる火の粉
class TitleEmberUI : public UIBase
{
public:
	void Init()override;
	void Update()override;
	void Draw()override;

	//斬った瞬間に強く吹かせる
	void Gust();

private:
	struct Ember
	{
		float x = 0.0f;
		float y = 0.0f;
		float vx = 0.0f;
		float vy = 0.0f;
		float size = 0.0f;
		float depth = 0.0f;//0=奥 1=手前
		float phase = 0.0f;//揺れと明滅
		int life = 0;
		int maxLife = 0;
	};

	void Spawn(Ember& e, bool anywhere);

	std::vector<Ember> m_embers;
	int m_frame = 0;
	float m_gust = 0.0f;
};
