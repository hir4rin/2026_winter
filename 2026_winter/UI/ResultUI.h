#pragma once
#include <string>
#include "Battle/ResultRecorder.h"

//リザルト画面の描画と出る順番
//座標は設計解像度(1280x720)で書き、描画時にScaleする
class ResultUI
{
public:
	ResultUI() = default;
	~ResultUI();

	void Init(const ResultData& data, const std::string& stageName, bool isNewRecord);
	void Update();
	void Draw();

	//1回目はためまで、2回目は最後まで飛ばす
	void Skip();
	bool IsFinished() const;

private:
	void DrawBackground();
	void DrawHeader();
	void DrawRails();
	void DrawAddRows();
	void DrawRateRows();
	void DrawDetailPanel();
	void DrawTotal();
	void DrawDark();
	void DrawStamp();
	void DrawNewRecord();
	void DrawGuide();
	//ランクの紋章の画像を読む
	static int LoadEmblem(int rank);

	//丸に漢字のアイコン(出るときに大きく出て縮む)
	void DrawIcon(float cx, float cy, const char* kanji, int start);
	//値・見出し・補足の3行
	void DrawRowText(float x, float y, const std::string& value, const std::string& valueSub, const char* label, const std::string& sub, int start, bool isPop);
	void DrawSelectBand(float x1, float y1, float x2, float y2, int start);
	void DrawFrame(float x1, float y1, float x2, float y2, int alpha);
	void DrawTiled(int handle, float x1, float y1, float x2, float y2, float tileSize, float u, float v, int r, int g, int b, int a);
	void DrawLabel(float x, float y, const std::string& str, unsigned int color, int font, int alpha);
	void DrawLabelRight(float x, float y, const std::string& str, unsigned int color, int font, int alpha);
	//線を start から len フレームで伸ばす
	void DrawGrowLineH(float x1, float x2, float y, int start, int len, unsigned int color);
	void DrawGrowLineV(float x, float y1, float y2, int start, int len, unsigned int color);

	float Rate(int start, int len) const;
	int FadeAlpha(int start) const;
	int CountUp(int value, int start, int len) const;
	float PopScale(int start) const;

	static std::string FormatNumber(int value);
	static std::string FormatTime(int frame);
	static std::string FormatRate(float rate);

	float ToScreenX(float x) const;
	float ToScreenY(float y) const;

	ResultData m_data;
	std::string m_stageName;
	bool m_isNewRecord = false;
	int m_rank = 0;
	int m_frame = 0;
	int m_bgFrame = 0;//背景のドットを流す用(スキップで飛ばない)
	float m_shakeX = 0.0f;
	float m_shakeY = 0.0f;

	int m_frameHandle = -1;
	int m_selectBaseHandle = -1;
	int m_halftoneHandle = -1;
	int m_lineFadeHandle = -1;
	int m_gridHandle = -1;
	int m_rainHandle = -1;
	int m_vignetteHandle = -1;
	int m_emblemHandle = -1;//ランクの紋章
	int m_buttonAHandle = -1;
	int m_buttonYHandle = -1;

	int m_headerFont = -1;
	int m_valueFont = -1;
	int m_pointFont = -1;
	int m_labelFont = -1;
	int m_smallFont = -1;
	int m_scoreFont = -1;
	int m_iconFont = -1;
};
