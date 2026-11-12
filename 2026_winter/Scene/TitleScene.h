#pragma once
#include "Scene.h"
#include "DxLib.h"

/// <summary>
/// タイトル画面。ロゴ「忍」を出して斬り、下半分が赤く染まって垂れる。Aでステージセレクトへ
/// </summary>
class TitleScene :
	public Scene
{
public:
	TitleScene(SceneController& controller);
	virtual ~TitleScene();

	void Update() override;
	void FadeInUpdate() override;
	void NormalUpdate() override;
	void FadeOutUpdate() override;

	void Draw() override;
	void FadeInDraw() override;
	void NormalDraw() override;
	void FadeOutDraw() override;

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
	//start フレームから length フレームかけて 0→1
	float Rate(int start, int length)const;
	//斬った瞬間からの画面の揺れ(設計解像度)
	float ShakeX()const;
	float ShakeY()const;
	void DrawFade();
	void DrawLogo();
	//忍をシェーダーで描く(斬る・染まる・垂れる)
	void DrawKanji(float cx, float cy, float scale, float alpha);
	void DrawCutGlow(float cx, float cy, float scale);
	void DrawFlash();
	void DrawPressButton();
	//斬った瞬間の閃光(Effekseer)を斬り線に合わせて出す
	void PlaySlashEffect();
	//ロゴ画像1ピクセルが画面で何ピクセルか
	float LogoScale()const;

	int m_fadeFrame = 0;//フェードの経過フレーム
	int m_frame = 0;//シーンが始まってからのフレーム(演出の進み)
	int m_backHandle = -1;
	int m_steelHandle = -1;
	int m_redHandle = -1;
	int m_cutHandle = -1;
	int m_vignetteHandle = -1;
	int m_font = -1;
	int m_shader = -1;
	int m_constantBuffer = -1;
	int m_slashEffect = -1;//エフェクトの素材
	int m_slashPlaying = -1;//再生中のエフェクト
	//斬り線(ロゴ画像のピクセル座標)
	float m_lineX = 0.0f, m_lineY = 0.0f, m_lineDirX = 1.0f, m_lineDirY = 0.0f;
};
