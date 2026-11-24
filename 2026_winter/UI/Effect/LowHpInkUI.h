#pragma once
#include "UI/UIBase.h"
#include <array>
#include <memory>

class Player;

//HPが低いときに、画面の端から墨がにじむ
//3Dの後・ほかのUIより先に描く(UIManagerの登録の順番)
class LowHpInkUI : public UIBase
{
public:
	explicit LowHpInkUI(std::weak_ptr<Player> player);
	~LowHpInkUI() override;

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	//HPから目標の効き具合(0～1)
	float CalcTargetRate() const;
	//ドクン・ドクンの形(phase:0～1)
	static float BeatShape(float phase);
	//画面全体に1回描く(pass 0:乗算 1:加算)
	void DrawPass(int pass);

	struct InkConstant
	{
		float param[4];//効き具合・心拍・時間(秒)・パス
		float screen[4];//横/縦
	};

	std::weak_ptr<Player> m_player;
	int m_shader = -1;
	std::array<int, 2> m_constantBuffers = { -1, -1 };//パスごと
	float m_rate = 0.0f;//今の効き具合
	float m_beatPhase = 0.0f;
	float m_beat = 0.0f;
	int m_frame = 0;
};
