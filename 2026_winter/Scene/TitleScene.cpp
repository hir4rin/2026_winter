#include "TitleScene.h"
#include "SceneController.h"
#include "StageSelectScene.h"
#include "Main/Input.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Render/ModelShaderManager.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <string>
#include <algorithm>

namespace
{
	const char* const kBackPath = "data/UI/Title/logo_back.png";
	const char* const kSteelPath = "data/UI/Title/logo_steel.png";
	const char* const kRedPath = "data/UI/Title/logo_red.png";
	const char* const kCutPath = "data/UI/Title/logo_cut.png";
	const char* const kCutLinePath = "data/UI/Title/logo_cut.csv";
	const char* const kVignettePath = "data/UI/Menu/vignette.png";
	const wchar_t* const kShaderPath = L"Render/Shader/TitleLogoPS.hlsl";
	const char* const kSlashEffectPath = "data/Effect/Title/TitleSlash.efk";
	constexpr float kSlashEffectLength = 10.0f;//エフェクトの刃の長さ(Effekseer の単位)
	constexpr float kSlashEffectPixel = 510.0f;//画面に出す刃の長さ(ロゴ画像のピクセル。斬り線は約445)
	const char* const kFontName = "游明朝";
	constexpr int kConstantSlot = 9;//TitleLogoPS.hlsl の register(b9)

	constexpr int kFadeFrame = 30;

	//ロゴ(設計解像度での位置と高さ)
	constexpr float kLogoX = 640.0f;
	constexpr float kLogoY = 320.0f;
	constexpr float kLogoHeight = 500.0f;
	constexpr int kLogoIntroFrame = 45;//少し大きい所から縮みながら出てくる
	constexpr float kLogoIntroScale = 1.06f;

	//斬る(フレームはシーンの始まりから)
	constexpr int kSlashFrame = 85;//斬り口の光が走り始める
	constexpr int kSlashWipeFrame = 5;//光が端から端まで走るフレーム
	constexpr int kSplitFrame = kSlashFrame + 4;//上がずれ始める
	constexpr int kSplitTime = 8;
	constexpr float kSplitShift = 18.0f;//上のずれ(画像のピクセル)
	constexpr float kSplitGap = 7.0f;//斬り口のすき間(画像のピクセル)
	constexpr int kCutGlowFadeTime = 50;//斬り口の光が弱まるまで
	constexpr int kCutGlowRest = 110;//弱まった後の光の濃さ(0〜255)

	//画面の白い光と揺れ
	constexpr int kFlashTime = 10;
	constexpr int kFlashAlpha = 170;
	constexpr int kShakeTime = 20;
	constexpr float kShakePower = 9.0f;//設計解像度のピクセル

	//赤く染まる・垂れる
	constexpr int kRedFrame = kSplitFrame + 6;
	constexpr int kRedTime = 45;
	constexpr float kRedBlur = 40.0f;//染まりの境目のぼかし(画像のピクセル)
	constexpr int kDripFrame = kRedFrame + 12;
	constexpr int kDripTime = 300;
	constexpr float kDripMaxLength = 190.0f;//垂れの最大の長さ(画像のピクセル)
	constexpr float kDripSpacing = 20.0f;//垂れの間隔
	constexpr float kDripWidth = 6.0f;//垂れの太さ(半分)
	constexpr float kDripExtra = 200.0f;//垂れが入るように板を画像より下に伸ばす

	//PRESS A BUTTON
	constexpr int kPressStartFrame = kRedFrame + 45;
	constexpr float kPressY = 660.0f;
	constexpr int kPressFontSize = 22;
	constexpr int kPressSpace = 8;
	constexpr float kBlinkSpeed = 0.06f;
	constexpr int kBgR = 12, kBgG = 13, kBgB = 17;
}

TitleScene::TitleScene(SceneController& controller) :Scene(controller)
{
	//最初のシーンなので、ここで入力を使えるようにしておく
	Input::GetInstance().Init();
	System::GetInstance().SetTimeScale(1.0f);

	m_updateFunc = static_cast<UpdateFunc_t>(&TitleScene::FadeInUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&TitleScene::FadeInDraw);

	//縮小で透明な部分がにじまないように乗算済みアルファで読む
	SetUsePremulAlphaConvertLoad(TRUE);
	m_backHandle = LoadGraph(kBackPath);
	m_steelHandle = LoadGraph(kSteelPath);
	m_redHandle = LoadGraph(kRedPath);
	m_cutHandle = LoadGraph(kCutPath);
	SetUsePremulAlphaConvertLoad(FALSE);
	m_vignetteHandle = LoadGraph(kVignettePath);
	LoadCutLine();

	m_shader = ModelShaderManager::LoadShaderWithMacro(kShaderPath, "ps_5_0", { {nullptr, nullptr} }, false);
	m_constantBuffer = CreateShaderConstantBuffer(sizeof(LogoConstant));

	Effekseer_Set2DSetting(Game::GetScreenWidth(), Game::GetScreenHeight());
	m_slashEffect = LoadEffekseerEffect(kSlashEffectPath);

	const float scale = Game::GetScale();
	m_font = CreateFontToHandle(kFontName, static_cast<int>(kPressFontSize * scale), 3, DX_FONTTYPE_ANTIALIASING_4X4);
	SetFontSpaceToHandle(static_cast<int>(kPressSpace * scale), m_font);
}

TitleScene::~TitleScene()
{
	DeleteGraph(m_backHandle);
	DeleteGraph(m_steelHandle);
	DeleteGraph(m_redHandle);
	DeleteGraph(m_cutHandle);
	DeleteGraph(m_vignetteHandle);
	DeleteFontToHandle(m_font);
	DeleteShader(m_shader);
	DeleteShaderConstantBuffer(m_constantBuffer);
	if (m_slashPlaying != -1)StopEffekseer2DEffect(m_slashPlaying);
	DeleteEffekseerEffect(m_slashEffect);
}

float TitleScene::LogoScale()const
{
	int w = 0, h = 0;
	GetGraphSize(m_steelHandle, &w, &h);
	if (h <= 0)return 0.0f;
	const float t = Rate(0, kLogoIntroFrame);
	const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	const float intro = kLogoIntroScale + (1.0f - kLogoIntroScale) * ease;
	return kLogoHeight / static_cast<float>(h) * Game::GetScale() * intro;
}

void TitleScene::PlaySlashEffect()
{
	int w = 0, h = 0;
	GetGraphSize(m_steelHandle, &w, &h);
	const float scale = LogoScale();
	//斬り線の点(画像の中心からのずれ)を画面の位置にする
	const float x = Game::ScaleX(kLogoX) + (m_lineX - w * 0.5f) * scale;
	const float y = Game::ScaleY(kLogoY) + (m_lineY - h * 0.5f) * scale;
	const float size = kSlashEffectPixel * scale / kSlashEffectLength;

	m_slashPlaying = PlayEffekseer2DEffect(m_slashEffect);
	SetPosPlayingEffekseer2DEffect(m_slashPlaying, x, y, 0.0f);
	SetRotationPlayingEffekseer2DEffect(m_slashPlaying, 0.0f, 0.0f, std::atan2(m_lineDirY, m_lineDirX));
	SetScalePlayingEffekseer2DEffect(m_slashPlaying, size, size, size);
}

void TitleScene::LoadCutLine()
{
	//1行目は見出し。x,y,dirX,dirY
	std::ifstream file(kCutLinePath);
	std::string line;
	if (!std::getline(file, line) || !std::getline(file, line))return;
	sscanf_s(line.c_str(), "%f,%f,%f,%f", &m_lineX, &m_lineY, &m_lineDirX, &m_lineDirY);
}

float TitleScene::Rate(int start, int length)const
{
	return std::clamp(static_cast<float>(m_frame - start) / static_cast<float>(length), 0.0f, 1.0f);
}

float TitleScene::ShakeX()const
{
	const int t = m_frame - kSplitFrame;
	if (t < 0 || t >= kShakeTime)return 0.0f;
	const float power = kShakePower * (1.0f - static_cast<float>(t) / kShakeTime);
	return std::sin(t * 2.3f) * power;
}

float TitleScene::ShakeY()const
{
	const int t = m_frame - kSplitFrame;
	if (t < 0 || t >= kShakeTime)return 0.0f;
	const float power = kShakePower * 0.6f * (1.0f - static_cast<float>(t) / kShakeTime);
	return std::cos(t * 3.1f) * power;
}

void TitleScene::Update()
{
	m_frame++;
	if (m_frame == kSlashFrame)PlaySlashEffect();
	(this->*m_updateFunc)();
	UpdateEffekseer2D();
}

void TitleScene::FadeInUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		m_fadeFrame = 0;
		m_updateFunc = static_cast<UpdateFunc_t>(&TitleScene::NormalUpdate);
		m_drawFunc = static_cast<DrawFunc_t>(&TitleScene::NormalDraw);
	}
}

void TitleScene::NormalUpdate()
{
	auto& input = Input::GetInstance();
	input.Update();
	if (!input.IsTriggered("A"))return;

	//演出の途中なら、斬り終わった所まで飛ばす
	if (m_frame < kPressStartFrame)
	{
		m_frame = kPressStartFrame;
		return;
	}
	m_fadeFrame = 0;
	m_updateFunc = static_cast<UpdateFunc_t>(&TitleScene::FadeOutUpdate);
	m_drawFunc = static_cast<DrawFunc_t>(&TitleScene::FadeOutDraw);
}

void TitleScene::FadeOutUpdate()
{
	m_fadeFrame++;
	if (m_fadeFrame >= kFadeFrame)
	{
		//このシーンはここで破棄されるので、呼んだ後は何もしない
		m_controller.ResetScene<StageSelectScene>();
	}
}

void TitleScene::Draw()
{
	(this->*m_drawFunc)();
}

void TitleScene::FadeInDraw()
{
	NormalDraw();
	DrawFade();
}

void TitleScene::NormalDraw()
{
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(kBgR, kBgG, kBgB), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_MUL, 255);
	DrawExtendGraph(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), m_vignetteHandle, FALSE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawLogo();
	SetDrawMode(DX_DRAWMODE_NEAREST);
	DrawEffekseer2D();
	DrawFlash();
	DrawPressButton();
}

void TitleScene::FadeOutDraw()
{
	NormalDraw();
	DrawFade();
}

void TitleScene::DrawLogo()
{
	int w = 0, h = 0;
	GetGraphSize(m_steelHandle, &w, &h);
	if (h <= 0)return;

	const float t = Rate(0, kLogoIntroFrame);
	const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	const float intro = kLogoIntroScale + (1.0f - kLogoIntroScale) * ease;
	const float scale = kLogoHeight / static_cast<float>(h) * Game::GetScale() * intro;
	const float cx = Game::ScaleX(kLogoX + ShakeX());
	const float cy = Game::ScaleY(kLogoY + ShakeY());

	SetDrawBlendMode(DX_BLENDMODE_PMA_ALPHA, static_cast<int>(255 * ease));
	DrawRotaGraphF(cx, cy, scale, 0.0, m_backHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	DrawKanji(cx, cy, scale, ease);
	DrawCutGlow(cx, cy, scale);
}

void TitleScene::DrawKanji(float cx, float cy, float scale, float alpha)
{
	int w = 0, h = 0;
	GetGraphSize(m_steelHandle, &w, &h);
	const float fw = static_cast<float>(w), fh = static_cast<float>(h);

	//上のずれ・すき間はすぐ開き切る(ease out)
	const float split = Rate(kSplitFrame, kSplitTime);
	const float splitEase = 1.0f - (1.0f - split) * (1.0f - split);

	auto* c = static_cast<LogoConstant*>(GetBufferShaderConstantBuffer(m_constantBuffer));
	c->size = { fw, fh, 1.0f / fw, 1.0f / fh };
	c->line = { m_lineX, m_lineY, m_lineDirX, m_lineDirY };
	c->cut = { kSplitShift * splitEase, kSplitGap * splitEase, Rate(kRedFrame, kRedTime), Rate(kDripFrame, kDripTime) };
	c->drip = { kDripMaxLength, kDripSpacing, kDripWidth, kRedBlur };
	UpdateShaderConstantBuffer(m_constantBuffer);
	SetShaderConstantBuffer(m_constantBuffer, DX_SHADERTYPE_PIXEL, kConstantSlot);

	//画像の中心が (cx, cy)。下に kDripExtra だけ伸ばす
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

void TitleScene::DrawCutGlow(float cx, float cy, float scale)
{
	if (m_frame < kSlashFrame)return;
	int w = 0, h = 0;
	GetGraphSize(m_cutHandle, &w, &h);

	//左から右へ走って、その後は少し弱まって残る
	const float wipe = Rate(kSlashFrame, kSlashWipeFrame);
	const float fadeRate = Rate(kSlashFrame + kSlashWipeFrame, kCutGlowFadeTime);
	const int alpha = static_cast<int>(255 + (kCutGlowRest - 255) * fadeRate);
	const float srcW = w * wipe;
	const float left = cx - w * 0.5f * scale;
	const float top = cy - h * 0.5f * scale;

	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, alpha);
	DrawRectExtendGraphF(left, top, left + srcW * scale, top + h * scale, 0, 0, static_cast<int>(srcW), h, m_cutHandle, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TitleScene::DrawFlash()
{
	const int t = m_frame - kSplitFrame;
	if (t < 0 || t >= kFlashTime)return;
	const float rate = 1.0f - static_cast<float>(t) / kFlashTime;
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kFlashAlpha * rate * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(255, 235, 230), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TitleScene::DrawPressButton()
{
	if (m_frame < kPressStartFrame)return;
	//ゆっくり明滅させる
	const float wave = 0.5f - 0.5f * std::cos((m_frame - kPressStartFrame) * kBlinkSpeed);
	const int alpha = static_cast<int>(40 + 215 * wave);

	const char* text = "PRESS  A  BUTTON";
	const int width = GetDrawStringWidthToHandle(text, static_cast<int>(strlen(text)), m_font);
	const int size = GetFontSizeToHandle(m_font);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringToHandle(static_cast<int>(Game::ScaleX(kLogoX)) - width / 2, static_cast<int>(Game::ScaleY(kPressY)) - size / 2,
		text, GetColor(225, 222, 218), m_font);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TitleScene::DrawFade()
{
	//フェードイン中は黒→透明、フェードアウト中は透明→黒
	float rate = static_cast<float>(m_fadeFrame) / static_cast<float>(kFadeFrame);
	if (m_updateFunc == static_cast<UpdateFunc_t>(&TitleScene::FadeInUpdate))
	{
		rate = 1.0f - rate;
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * rate));
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
