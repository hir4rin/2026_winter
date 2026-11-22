#pragma once
#include "UI/UIBase.h"
#include "DxLib.h"

//タイトルロゴ「忍」//3Dの1点に置いてカメラで写す。斬る・赤く染まる・垂れる
class TitleLogoUI : public UIBase
{
public:
	~TitleLogoUI();
	void Init()override;
	void Update()override;
	void Draw()override;

	//斬る(斬り口の光・閃光から始まる)
	void Slash();
	//斬り終わった所まで飛ばす
	void Skip();

private:
	//TitleLogoPS.hlsl の cbuffer と同じ並び
	struct LogoConstant
	{
		FLOAT4 size;
		FLOAT4 line;
		FLOAT4 cut;
		FLOAT4 drip;
	};

	void LoadCutLine();
	//斬ってから start フレーム後から length かけて 0→1
	float SlashRate(int start, int length)const;
	//ロゴの中心の画面の位置と、画像1ピクセルが画面で何ピクセルか
	void GetScreen(float& cx, float& cy, float& scale)const;
	void DrawKanji(float cx, float cy, float scale, float alpha);
	void DrawCutGlow(float cx, float cy, float scale);
	void PlaySlashEffect();

	int m_backHandle = -1;
	int m_steelHandle = -1;
	int m_redHandle = -1;
	int m_cutHandle = -1;
	int m_shader = -1;
	int m_constantBuffer = -1;
	int m_slashEffect = -1;
	int m_slashPlaying = -1;
	int m_width = 0;
	int m_height = 0;

	int m_frame = 0;//出てからのフレーム
	int m_slashFrame = -1;//斬ってからのフレーム(-1は斬る前)
	//斬り線(ロゴ画像のピクセル座標)
	float m_lineX = 0.0f, m_lineY = 0.0f, m_lineDirX = 1.0f, m_lineDirY = 0.0f;
};
