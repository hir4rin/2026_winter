#pragma once
#include "UI/UIBase.h"

//エンディングのクレジット(Xから文字に化ける赤い文字)とタイトルロゴ
//出す時間は TunnelEnding が BattleManager::EndingScreen に書く
class EndingUI : public UIBase
{
public:
	~EndingUI();
	void Init()override;
	void Update()override;
	void Draw()override;

private:
	void DrawCredit(int index, int frame);
	//1行分。delay:この行が出始めるフレーム
	void DrawCreditLine(const char* text, float centerX, float y, int font, int frame, int delay, int seed);
	//文字を赤く、色ずれ・横のにじみつきで描く
	void DrawGlyph(float x, float y, const char* glyph, int font, float scale, float alpha);
	//赤い四角のノイズ
	void DrawGlitchBlocks(float centerX, float centerY, float halfWidth, float halfHeight, int frame, int seed);

	void DrawLogo(int frame);
	//blendGraph の明るい所から順に rate の分だけ見せる(-1ならそのまま)
	void DrawLogoGraph(int handle, int blendGraph, float rate, float offsetX = 0.0f);

	//無線の字幕
	void DrawRadio(int index, int frame);

	static int MakeBlendGraph(int width, int height, bool isWipe);
	static float Random01(int a, int b, int c);

private:
	int m_roleFont = -1;
	int m_nameFont = -1;
	int m_logoHandle = -1;//完成形
	int m_redHandle = -1;//赤い忍
	int m_steelHandle = -1;//鋼の忍
	int m_backHandle = -1;//後ろの帯・筆
	int m_cutHandle = -1;//斬り口の光
	int m_noiseHandle = -1;//まだらに消す・出す
	int m_wipeHandle = -1;//左から出す

	int m_creditIndex = -1;
	int m_creditFrame = 0;
	int m_logoFrame = -1;

	int m_radioFont = -1;
};
