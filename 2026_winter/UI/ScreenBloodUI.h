#pragma once
#include "UIBase.h"
#include <array>
#include <vector>

//暗殺・確殺のときに、画面(レンズ)に血を付けるUI
//血殺の血のUIと同じ、べた塗りの墨の飛沫。HUDより先に描く(UIManagerの登録の順番)
class ScreenBloodUI : public UIBase
{
public:
	ScreenBloodUI() = default;
	~ScreenBloodUI() override;

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	//血を付ける
	void Spawn();
	//飛沫を1つ足す。rootX/Y:飛沫の根元(設計解像度)、angle:飛ぶ向き
	void AddStain(int handle, float rootOffset, float rootX, float rootY, float angle, float scale, int delay);
	void DrawVignette();
	void DrawStains();
	//min～maxの乱数
	static float RandRange(float min, float max);

	//画面に付いた血1つ分
	struct Stain
	{
		int handle = -1;
		float x = 0.0f;//画像の中心(設計解像度)
		float y = 0.0f;
		float scale = 1.0f;
		float angle = 0.0f;
		float rootDistance = 0.0f;//画像の中心から根元までの距離(設計解像度)
		int delay = 0;//出るまでのフレーム
		int frame = 0;//経過フレーム
	};
	std::vector<Stain> m_stains;

	static constexpr int kSplatNum = 4;
	static constexpr int kSprayNum = 3;
	std::array<int, kSplatNum> m_splatHandles = { -1, -1, -1, -1 };
	std::array<int, kSprayNum> m_sprayHandles = { -1, -1, -1 };
	int m_vignetteHandle = -1;
	int m_vignetteFrame = -1;//経過フレーム(-1:出ていない)
};
