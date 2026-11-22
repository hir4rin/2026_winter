#include "TitleLogoUI.h"
#include "EffekseerForDXLib.h"
#include "Main/Game.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

namespace
{
	const char* const kBackPath = "data/UI/Title/logo_back.png";
	const char* const kSteelPath = "data/UI/Title/logo_steel.png";
	const char* const kRedPath = "data/UI/Title/logo_red.png";
	const char* const kCutPath = "data/UI/Title/logo_cut.png";
	const char* const kCutLinePath = "data/UI/Title/logo_cut.csv";
	const wchar_t* const kShaderPath = L"Render/Shader/TitleLogoPS.hlsl";
	const char* const kSlashEffectPath = "data/Effect/Title/TitleSlash.efk";
	constexpr float kSlashEffectLength = 10.0f;//エフェクトの刃の長さ(Effekseer の単位)
	constexpr float kSlashEffectPixel = 510.0f;//画面に出す刃の長さ(ロゴ画像のピクセル)
	constexpr int kConstantSlot = 9;//register(b9)

	//置く場所(タイトルカメラで1単位=設計解像度の1ピクセル)
	constexpr float kWorldX = -110.0f;//右に顔を置くので少し左
	constexpr float kWorldY = 40.0f;
	constexpr float kHeight = 500.0f;
	constexpr int kAppearFrame = 45;

	//斬ってからのフレーム
	constexpr int kWipeTime = 5;//斬り口の光が走る
	constexpr int kSplitDelay = 4;
	constexpr int kSplitTime = 8;
	constexpr float kSplitShift = 18.0f;//上のずれ(画像のピクセル)
	constexpr float kSplitGap = 7.0f;//すき間(画像のピクセル)
	constexpr int kCutGlowFadeTime = 50;
	constexpr int kCutGlowRest = 110;//弱まった後の光の濃さ
	constexpr int kRedDelay = kSplitDelay + 6;
	constexpr int kRedTime = 45;
	constexpr float kRedBlur = 40.0f;
	constexpr int kDripDelay = kRedDelay + 12;
	constexpr int kDripTime = 300;
	constexpr float kDripMaxLength = 190.0f;
	constexpr float kDripSpacing = 20.0f;
	constexpr float kDripWidth = 6.0f;
	constexpr float kDripExtra = 200.0f;//垂れが入るように板を下に伸ばす
	constexpr int kSkipSlashFrame = kRedDelay + kRedTime;//飛ばしたら染まり切った所
}

TitleLogoUI::~TitleLogoUI()
{
	DeleteGraph(m_backHandle);
	DeleteGraph(m_steelHandle);
	DeleteGraph(m_redHandle);
	DeleteGraph(m_cutHandle);
	DeleteShader(m_shader);
	DeleteShaderConstantBuffer(m_constantBuffer);
	if (m_slashPlaying != -1)StopEffekseer2DEffect(m_slashPlaying);
	DeleteEffekseerEffect(m_slashEffect);
}

void TitleLogoUI::Init()
{
	//縮小で透明な所がにじまないように乗算済みアルファ
	SetUsePremulAlphaConvertLoad(TRUE);
	m_backHandle = LoadGraph(kBackPath);
	m_steelHandle = LoadGraph(kSteelPath);
	m_redHandle = LoadGraph(kRedPath);
	m_cutHandle = LoadGraph(kCutPath);
	SetUsePremulAlphaConvertLoad(FALSE);
	GetGraphSize(m_steelHandle, &m_width, &m_height);
	LoadCutLine();

	m_shader = ModelShaderManager::LoadShaderWithMacro(kShaderPath, "ps_5_0", { {nullptr, nullptr} }, false);
	m_constantBuffer = CreateShaderConstantBuffer(sizeof(LogoConstant));

	Effekseer_Set2DSetting(Game::GetScreenWidth(), Game::GetScreenHeight());
	m_slashEffect = LoadEffekseerEffect(kSlashEffectPath);

	m_frame = 0;
	m_slashFrame = -1;
	m_isVisible = true;
}

void TitleLogoUI::Update()
{
	if (m_frame < kAppearFrame)m_frame++;
	if (m_slashFrame >= 0)m_slashFrame++;
	UpdateEffekseer2D();
}

void TitleLogoUI::Draw()
{
	if (!m_isVisible || m_height <= 0)return;
	float cx = 0.0f, cy = 0.0f, scale = 0.0f;
	GetScreen(cx, cy, scale);

	const float t = static_cast<float>(m_frame) / kAppearFrame;
	const float alpha = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

	SetDrawMode(DX_DRAWMODE_BILINEAR);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * alpha));
	DrawRotaGraphF(cx, cy, scale, 0.0, m_backHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawKanji(cx, cy, scale, alpha);
	DrawCutGlow(cx, cy, scale);
	SetDrawMode(DX_DRAWMODE_NEAREST);

	DrawEffekseer2D();
}

void TitleLogoUI::Slash()
{
	m_slashFrame = 0;
	PlaySlashEffect();
}

void TitleLogoUI::Skip()
{
	m_frame = kAppearFrame;
	if (m_slashFrame < kSkipSlashFrame)m_slashFrame = kSkipSlashFrame;
}

void TitleLogoUI::LoadCutLine()
{
	//1行目は見出し。x,y,dirX,dirY
	std::ifstream file(kCutLinePath);
	std::string line;
	if (!std::getline(file, line) || !std::getline(file, line))return;
	sscanf_s(line.c_str(), "%f,%f,%f,%f", &m_lineX, &m_lineY, &m_lineDirX, &m_lineDirY);
}

float TitleLogoUI::SlashRate(int start, int length)const
{
	if (m_slashFrame < 0)return 0.0f;
	return std::clamp(static_cast<float>(m_slashFrame - start) / static_cast<float>(length), 0.0f, 1.0f);
}

void TitleLogoUI::GetScreen(float& cx, float& cy, float& scale)const
{
	//今のカメラで写した位置と、1単位の画面の長さ
	const VECTOR center = ConvWorldPosToScreenPos(VGet(kWorldX, kWorldY, 0.0f));
	const VECTOR up = ConvWorldPosToScreenPos(VGet(kWorldX, kWorldY + 1.0f, 0.0f));
	cx = center.x;
	cy = center.y;
	scale = (center.y - up.y) * kHeight / static_cast<float>(m_height);
}

void TitleLogoUI::DrawKanji(float cx, float cy, float scale, float alpha)
{
	const float fw = static_cast<float>(m_width), fh = static_cast<float>(m_height);

	//ずれ・すき間はすぐ開き切る
	const float split = SlashRate(kSplitDelay, kSplitTime);
	const float splitEase = 1.0f - (1.0f - split) * (1.0f - split);

	auto* c = static_cast<LogoConstant*>(GetBufferShaderConstantBuffer(m_constantBuffer));
	c->size = { fw, fh, 1.0f / fw, 1.0f / fh };
	c->line = { m_lineX, m_lineY, m_lineDirX, m_lineDirY };
	c->cut = { kSplitShift * splitEase, kSplitGap * splitEase, SlashRate(kRedDelay, kRedTime), SlashRate(kDripDelay, kDripTime) };
	c->drip = { kDripMaxLength, kDripSpacing, kDripWidth, kRedBlur };
	UpdateShaderConstantBuffer(m_constantBuffer);
	SetShaderConstantBuffer(m_constantBuffer, DX_SHADERTYPE_PIXEL, kConstantSlot);

	const float left = cx - fw * 0.5f * scale;
	const float top = cy - fh * 0.5f * scale;
	const float right = cx + fw * 0.5f * scale;
	const float bottom = top + (fh + kDripExtra) * scale;
	const float vMax = (fh + kDripExtra) / fh;

	VERTEX2DSHADER v[4] = {};
	const float pos[4][4] = { {left, top, 0.0f, 0.0f}, {right, top, 1.0f, 0.0f}, {left, bottom, 0.0f, vMax}, {right, bottom, 1.0f, vMax} };
	for (int i = 0; i < 4; ++i)
	{
		v[i].pos = VGet(pos[i][0], pos[i][1], 0.0f);
		v[i].rhw = 1.0f;
		v[i].dif = GetColorU8(255, 255, 255, static_cast<int>(255 * alpha));
		v[i].spc = GetColorU8(0, 0, 0, 0);
		v[i].u = pos[i][2];
		v[i].v = pos[i][3];
		v[i].su = v[i].u;
		v[i].sv = v[i].v;
	}

	SetUsePixelShader(m_shader);
	SetUseTextureToShader(0, m_steelHandle);
	SetUseTextureToShader(1, m_redHandle);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, 255);
	DrawPrimitive2DToShader(v, 4, DX_PRIMTYPE_TRIANGLESTRIP);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetUseTextureToShader(0, -1);
	SetUseTextureToShader(1, -1);
}

void TitleLogoUI::DrawCutGlow(float cx, float cy, float scale)
{
	if (m_slashFrame < 0)return;
	int w = 0, h = 0;
	GetGraphSize(m_cutHandle, &w, &h);

	//左から右へ走って、少し弱まって残る
	const float wipe = SlashRate(0, kWipeTime);
	const float fadeRate = SlashRate(kWipeTime, kCutGlowFadeTime);
	const int alpha = static_cast<int>(255 + (kCutGlowRest - 255) * fadeRate);
	const float srcW = w * wipe;
	const float left = cx - w * 0.5f * scale;
	const float top = cy - h * 0.5f * scale;

	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, alpha);
	DrawRectExtendGraphF(left, top, left + srcW * scale, top + h * scale, 0, 0, static_cast<int>(srcW), h, m_cutHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TitleLogoUI::PlaySlashEffect()
{
	float cx = 0.0f, cy = 0.0f, scale = 0.0f;
	GetScreen(cx, cy, scale);
	//斬り線の点を画面の位置にする
	const float x = cx + (m_lineX - m_width * 0.5f) * scale;
	const float y = cy + (m_lineY - m_height * 0.5f) * scale;
	const float size = kSlashEffectPixel * scale / kSlashEffectLength;

	m_slashPlaying = PlayEffekseer2DEffect(m_slashEffect);
	SetPosPlayingEffekseer2DEffect(m_slashPlaying, x, y, 0.0f);
	SetRotationPlayingEffekseer2DEffect(m_slashPlaying, 0.0f, 0.0f, std::atan2(m_lineDirY, m_lineDirX));
	SetScalePlayingEffekseer2DEffect(m_slashPlaying, size, size, size);
}
