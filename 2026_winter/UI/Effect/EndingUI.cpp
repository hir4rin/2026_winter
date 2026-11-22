#include "EndingUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>

namespace
{
	//クレジット
	const char* const kCreditFontName = "Times New Roman";
	constexpr int kRoleFontSize = 26;
	constexpr int kNameFontSize = 34;
	constexpr int kFontThick = 3;
	constexpr int kFontSpace = 6;//字間
	constexpr float kCreditX = 640.0f;
	constexpr float kRoleY = 292.0f;
	constexpr float kNameY = 336.0f;
	constexpr int kNameDelay = 3;//2行目は少し遅れる

	constexpr int kGlitchInFrame = 8;//出始めの赤い四角
	constexpr int kXAppearFrame = 4;//Xが並ぶ
	constexpr int kResolveFrame = 14;//Xが文字に変わり始める
	constexpr float kResolveStep = 0.9f;//1文字ごとの遅れ
	constexpr float kResolveRandom = 6.0f;//変わるタイミングのばらつき
	constexpr float kXScale = 1.25f;//Xは少し大きい
	constexpr int kCreditExitFrame = 200;//消え始め(赤い四角が走る)
	constexpr int kCreditHideFrame = 204;//文字が消える
	constexpr int kCreditEndFrame = 208;
	constexpr int kGlitchBlockNum = 4;
	constexpr int kScanlineAlpha = 28;//文字の上の暗い横線

	constexpr int kTextR = 240;
	constexpr int kTextG = 50;
	constexpr int kTextB = 75;

	//ロゴ
	const char* const kLogoPath = "data/UI/Title/logo.png";
	const char* const kRedPath = "data/UI/Title/logo_red.png";
	const char* const kSteelPath = "data/UI/Title/logo_steel.png";
	const char* const kBackPath = "data/UI/Title/logo_back.png";
	const char* const kCutPath = "data/UI/Title/logo_cut.png";
	constexpr float kLogoX = 640.0f;
	constexpr float kLogoY = 330.0f;
	constexpr float kLogoHeight = 400.0f;

	constexpr int kLogoWipeEnd = 9;//赤い忍が左から出る
	constexpr int kLogoRedEnd = 30;//赤がまだらに抜けて鋼になる
	constexpr int kLogoCutStart = 30;//斬り口の光
	constexpr int kLogoCutWipe = 5;
	constexpr int kLogoCutEnd = 46;
	constexpr int kLogoFullStart = 34;//完成形がまだらに出る
	constexpr int kLogoFullEnd = 46;
	constexpr int kSplatStart = 34;//血のしずくが飛ぶ
	constexpr int kSplatLife = 30;
	constexpr int kSplatNum = 24;
	constexpr int kLogoSmearStart = 330;//赤くなって横にぶれる
	constexpr int kLogoOutStart = 336;//まだらに消える
	constexpr int kLogoOutLength = 12;
	constexpr int kBackLingerLength = 30;//後ろの筆だけ赤く少し残る
	constexpr float kLogoHoldScale = 0.02f;//出ている間にゆっくり大きくなる分
	constexpr int kLogoHoldLength = 300;

	constexpr int kNoiseCellLarge = 48;//まだらの大きさ
	constexpr int kNoiseCellSmall = 12;
	constexpr float kWipeJitter = 0.12f;//ワイプの境目のぎざぎざ

	const char* const kUiFontName = "游明朝";
	constexpr int kRadioFontSize = 26;
	//無線の字幕
	constexpr float kRadioY = 600.0f;
	constexpr int kRadioFadeFrame = 10;
	constexpr int kRadioShowFrame = 300;//出ている長さ
	constexpr int kRadioBandAlpha = 150;
	constexpr int kSpeakerR = 120;
	constexpr int kSpeakerG = 220;
	constexpr int kSpeakerB = 255;
	//TunnelEndingの番号と合わせる
	const char* const kRadioSpeaker = "ウミ";
	const char* const kRadioLines[] =
	{
		"ミサイル！　囲まれてる！",
		"……応答して！",
	};
}

EndingUI::~EndingUI()
{
	DeleteFontToHandle(m_roleFont);
	DeleteFontToHandle(m_nameFont);
	DeleteGraph(m_logoHandle);
	DeleteGraph(m_redHandle);
	DeleteGraph(m_steelHandle);
	DeleteGraph(m_backHandle);
	DeleteGraph(m_cutHandle);
	DeleteGraph(m_noiseHandle);
	DeleteGraph(m_wipeHandle);
	DeleteFontToHandle(m_radioFont);
}

void EndingUI::Init()
{
	m_isVisible = true;
	const float scale = Game::GetScale();
	m_roleFont = CreateFontToHandle(kCreditFontName, static_cast<int>(kRoleFontSize * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_4X4);
	m_nameFont = CreateFontToHandle(kCreditFontName, static_cast<int>(kNameFontSize * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_4X4);
	SetFontSpaceToHandle(static_cast<int>(kFontSpace * scale), m_roleFont);
	SetFontSpaceToHandle(static_cast<int>(kFontSpace * scale), m_nameFont);

	m_logoHandle = LoadGraph(kLogoPath);
	m_redHandle = LoadGraph(kRedPath);
	m_steelHandle = LoadGraph(kSteelPath);
	m_backHandle = LoadGraph(kBackPath);
	m_cutHandle = LoadGraph(kCutPath);
	int w = 0;
	int h = 0;
	GetGraphSize(m_logoHandle, &w, &h);
	m_noiseHandle = MakeBlendGraph(w, h, false);
	m_wipeHandle = MakeBlendGraph(w, h, true);

	m_radioFont = CreateFontToHandle(kUiFontName, static_cast<int>(kRadioFontSize * scale), 3, DX_FONTTYPE_ANTIALIASING_EDGE_4X4);
}

void EndingUI::Update()
{
	auto battle = System::GetInstance().GetBattleMgr();
	if (!battle)return;
	const auto& screen = battle->GetEndingScreen();
	m_creditIndex = screen.creditIndex;
	m_creditFrame = screen.creditFrame;
	m_logoFrame = screen.logoFrame;
}

void EndingUI::Draw()
{
	auto battle = System::GetInstance().GetBattleMgr();
	if (m_creditIndex >= 0) DrawCredit(m_creditIndex, m_creditFrame);
	if (m_logoFrame >= 0) DrawLogo(m_logoFrame);
	if (battle)
	{
		const auto& screen = battle->GetEndingScreen();
		if (screen.radioIndex >= 0) DrawRadio(screen.radioIndex, screen.radioFrame);
		if (screen.blackRate > 0.0f)
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * screen.blackRate));
			DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(0, 0, 0), TRUE);
		}
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
}

void EndingUI::DrawRadio(int index, int frame)
{
	if (index < 0 || index >= static_cast<int>(std::size(kRadioLines)))return;
	if (frame >= kRadioShowFrame)return;
	const float fadeIn = std::clamp(static_cast<float>(frame) / kRadioFadeFrame, 0.0f, 1.0f);
	const float fadeOut = std::clamp(static_cast<float>(kRadioShowFrame - frame) / kRadioFadeFrame, 0.0f, 1.0f);
	const float alpha = (std::min)(fadeIn, fadeOut);
	const float s = Game::GetScale();

	const char* line = kRadioLines[index];
	const int speakerWidth = GetDrawStringWidthToHandle(kRadioSpeaker, static_cast<int>(strlen(kRadioSpeaker)), m_radioFont);
	const int lineWidth = GetDrawStringWidthToHandle(line, static_cast<int>(strlen(line)), m_radioFont);
	const float gap = 18.0f * s;
	const float total = speakerWidth + gap + lineWidth;
	const float left = Game::GetScreenWidth() * 0.5f - total * 0.5f;
	const float y = kRadioY * s;
	const float fontH = kRadioFontSize * s;

	//後ろの暗い帯
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(kRadioBandAlpha * alpha));
	DrawBoxAA(left - 30.0f * s, y - fontH * 0.45f, left + total + 30.0f * s, y + fontH * 1.45f, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	DrawStringFToHandle(left, y, kRadioSpeaker, GetColor(kSpeakerR, kSpeakerG, kSpeakerB), m_radioFont, GetColor(0, 0, 0));
	DrawStringFToHandle(left + speakerWidth + gap, y, line, GetColor(255, 255, 255), m_radioFont, GetColor(0, 0, 0));
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void EndingUI::DrawCredit(int index, int frame)
{
	//中身は仮
	static const char* const kCredits[][2] =
	{
		{ "Presented by", "SHINOBI PROJECT" },
		{ "Original Concept by", "TEAM WINTER 2026" },
		{ "Developed by", "KAGE NO STUDIO" },
	};
	constexpr int kCreditNum = static_cast<int>(sizeof(kCredits) / sizeof(kCredits[0]));
	if (index >= kCreditNum || frame >= kCreditEndFrame)return;

	const float centerX = Game::ScaleX(kCreditX);
	const float roleY = Game::ScaleY(kRoleY);
	const float nameY = Game::ScaleY(kNameY);
	DrawCreditLine(kCredits[index][0], centerX, roleY, m_roleFont, frame, 0, index * 2);
	DrawCreditLine(kCredits[index][1], centerX, nameY, m_nameFont, frame, kNameDelay, index * 2 + 1);

	//出始めと消え際に赤い四角
	const float halfHeight = (nameY - roleY) + Game::ScaleY(20.0f);
	const float centerY = (roleY + nameY) * 0.5f + Game::ScaleY(14.0f);
	const float halfWidth = Game::ScaleX(260.0f);
	if (frame < kGlitchInFrame || (frame >= kCreditExitFrame && frame < kCreditEndFrame))
	{
		DrawGlitchBlocks(centerX, centerY, halfWidth, halfHeight, frame, index);
	}
}

void EndingUI::DrawCreditLine(const char* text, float centerX, float y, int font, int frame, int delay, int seed)
{
	if (frame < kXAppearFrame + delay || frame >= kCreditHideFrame)return;

	const int length = static_cast<int>(strlen(text));
	const float width = static_cast<float>(GetDrawStringWidthToHandle(text, length, font));
	const float height = static_cast<float>(GetFontSizeToHandle(font));
	const float startX = centerX - width * 0.5f;

	for (int i = 0; i < length; ++i)
	{
		if (text[i] == ' ')continue;
		const float x = startX + GetDrawStringWidthToHandle(text, i, font);
		const char glyph[2] = { text[i], '\0' };
		const float resolve = kResolveFrame + delay + i * kResolveStep + Random01(seed, i, 0) * kResolveRandom;
		if (frame >= resolve)
		{
			DrawGlyph(x, y, glyph, font, 1.0f, 1.0f);
			continue;
		}
		//まだXのまま(少し大きく、ちらつく)
		const float glyphWidth = static_cast<float>(GetDrawStringWidthToHandle(glyph, 1, font));
		const float xWidth = GetDrawStringWidthToHandle("X", 1, font) * kXScale;
		const float xX = x + (glyphWidth - xWidth) * 0.5f;
		const float xY = y - height * (kXScale - 1.0f) * 0.5f;
		const float flicker = 0.45f + 0.55f * Random01(seed, i, frame / 2 + 1);
		DrawGlyph(xX, xY, "X", font, kXScale, flicker);
	}

	//横線のノイズ(暗い走査線と、はみ出す赤い線)
	const float scale = Game::GetScale();
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, kScanlineAlpha);
	for (float lineY = y; lineY < y + height; lineY += 3.0f * scale)
	{
		DrawBoxAA(startX, lineY, startX + width, lineY + scale, 0x000000, TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(60 + 60 * Random01(seed, frame / 3, 7)));
	for (int i = 0; i < 2; ++i)
	{
		const float lineY = y + height * (0.3f + 0.4f * Random01(seed, i, 11));
		const float extend = width * (0.1f + 0.25f * Random01(seed, i, 13));
		DrawBoxAA(startX - extend, lineY, startX + width + extend, lineY + scale, GetColor(150, 20, 30), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void EndingUI::DrawGlyph(float x, float y, const char* glyph, int font, float scale, float alpha)
{
	const float s = Game::GetScale();
	//横ににじむ赤い光
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(70 * alpha));
	for (float dx : { -12.0f, -6.0f, 6.0f, 12.0f })
	{
		DrawExtendStringFToHandle(x + dx * s, y, scale, scale, glyph, GetColor(110, 8, 20), font);
	}
	//色ずれ
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(150 * alpha));
	DrawExtendStringFToHandle(x + 2.0f * s, y, scale, scale, glyph, GetColor(0, 70, 140), font);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * alpha));
	DrawExtendStringFToHandle(x, y, scale, scale, glyph, GetColor(kTextR, kTextG, kTextB), font);
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(120 * alpha));
	DrawExtendStringFToHandle(x - 2.0f * s, y, scale, scale, glyph, GetColor(120, 0, 60), font);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void EndingUI::DrawGlitchBlocks(float centerX, float centerY, float halfWidth, float halfHeight, int frame, int seed)
{
	const float s = Game::GetScale();
	const int step = frame / 2;//2フレームごとに変える
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 230);
	for (int i = 0; i < kGlitchBlockNum; ++i)
	{
		const float w = (20.0f + 140.0f * Random01(seed, step, i * 4 + 1)) * s;
		const float h = (6.0f + 24.0f * Random01(seed, step, i * 4 + 2)) * s;
		const float x = centerX + (Random01(seed, step, i * 4 + 3) * 2.0f - 1.0f) * halfWidth;
		const float y = centerY + (Random01(seed, step, i * 4 + 4) * 2.0f - 1.0f) * halfHeight;
		DrawBoxAA(x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f, GetColor(kTextR, 40, 60), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void EndingUI::DrawLogo(int frame)
{
	const float t = static_cast<float>(frame);
	auto rate = [t](float start, float end) { return std::clamp((t - start) / (end - start), 0.0f, 1.0f); };

	//出始め:赤い忍が左から → 赤がまだらに抜けて鋼に
	if (frame < kLogoFullEnd)
	{
		if (frame >= kLogoWipeEnd) DrawLogoGraph(m_steelHandle, -1, 1.0f);
		const float redRate = (frame < kLogoWipeEnd) ? rate(0.0f, kLogoWipeEnd) : 1.0f - rate(kLogoWipeEnd, kLogoRedEnd);
		if (redRate > 0.0f) DrawLogoGraph(m_redHandle, (frame < kLogoWipeEnd) ? m_wipeHandle : m_noiseHandle, redRate);
	}
	//斬って完成形がまだらに出る
	if (frame >= kLogoFullStart && frame < kLogoOutStart + kLogoOutLength)
	{
		const float inRate = rate(kLogoFullStart, kLogoFullEnd);
		const float outRate = 1.0f - rate(kLogoOutStart, kLogoOutStart + kLogoOutLength);
		const bool isSmear = frame >= kLogoSmearStart;
		if (isSmear)
		{
			//赤くなって横にぶれる
			SetDrawBright(255, 70, 70);
			const float smear = (t - kLogoSmearStart) * 4.0f * Game::GetScale();
			SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
			for (int k = 1; k <= 3; ++k)
			{
				DrawLogoGraph(m_logoHandle, m_noiseHandle, outRate, smear * k);
				DrawLogoGraph(m_logoHandle, m_noiseHandle, outRate, -smear * k);
			}
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
		}
		DrawLogoGraph(m_logoHandle, m_noiseHandle, (std::min)(inRate, outRate));
		SetDrawBright(255, 255, 255);
	}
	//斬り口の光
	if (frame >= kLogoCutStart && frame < kLogoCutEnd)
	{
		const float fade = 1.0f - rate(kLogoCutStart + kLogoCutWipe, kLogoCutEnd);
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * fade));
		DrawLogoGraph(m_cutHandle, m_wipeHandle, rate(kLogoCutStart, kLogoCutStart + kLogoCutWipe));
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	}
	//血のしずく
	if (frame >= kSplatStart && frame < kSplatStart + kSplatLife)
	{
		int w = 0;
		int h = 0;
		GetGraphSize(m_logoHandle, &w, &h);
		const float scale = Game::ScaleY(kLogoHeight) / h;
		const float life = rate(kSplatStart, kSplatStart + kSplatLife);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * (1.0f - life)));
		for (int i = 0; i < kSplatNum; ++i)
		{
			//後ろの赤い筆のあたりから外へ
			const float px = (480.0f + 140.0f * Random01(i, 1, 0) - w * 0.5f) * scale;
			const float py = (60.0f + 460.0f * Random01(i, 2, 0) - h * 0.5f) * scale;
			const float dirY = Random01(i, 3, 0) - 0.5f;
			const float speed = (3.0f + 6.0f * Random01(i, 4, 0)) * Game::GetScale();
			const float move = speed * (t - kSplatStart);
			const float radius = (2.0f + 4.0f * Random01(i, 5, 0)) * Game::GetScale() * (1.0f - life * 0.5f);
			DrawCircleAA(Game::ScaleX(kLogoX) + px + move, Game::ScaleY(kLogoY) + py + move * dirY, radius, 12, GetColor(180, 10, 25), TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	//消えた後、後ろの筆だけ赤い光で少し残る
	if (frame >= kLogoOutStart)
	{
		const float lingerRate = 1.0f - rate(kLogoOutStart, kLogoOutStart + kBackLingerLength);
		SetDrawBright(255, 40, 40);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
		DrawLogoGraph(m_backHandle, m_noiseHandle, lingerRate);
		SetDrawBright(255, 255, 255);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void EndingUI::DrawLogoGraph(int handle, int blendGraph, float rate, float offsetX)
{
	if (rate <= 0.0f)return;
	int w = 0;
	int h = 0;
	GetGraphSize(handle, &w, &h);
	const float hold = std::clamp(static_cast<float>(m_logoFrame - kLogoFullEnd) / kLogoHoldLength, 0.0f, 1.0f);
	const double scale = Game::ScaleY(kLogoHeight) / h * (1.0f + kLogoHoldScale * hold);
	if (blendGraph != -1 && rate < 1.0f)
	{
		//明るい所(値が大きい所)から見せる
		SetBlendGraphParam(blendGraph, DX_BLENDGRAPHTYPE_WIPE, static_cast<int>(255 * (1.0f - rate)), 64);
	}
	DrawRotaGraphF(Game::ScaleX(kLogoX) + offsetX, Game::ScaleY(kLogoY), scale, 0.0, handle, TRUE);
	SetBlendGraph(-1, 0, 0);
}

int EndingUI::MakeBlendGraph(int width, int height, bool isWipe)
{
	if (width <= 0 || height <= 0)return -1;
	//格子に乱数を置いて、間をなめらかにつないだまだら模様
	auto valueNoise = [](float x, float y, int cell, int seed)
		{
			const int cx = static_cast<int>(x) / cell;
			const int cy = static_cast<int>(y) / cell;
			float fx = (x - cx * cell) / cell;
			float fy = (y - cy * cell) / cell;
			fx = fx * fx * (3.0f - 2.0f * fx);
			fy = fy * fy * (3.0f - 2.0f * fy);
			const float a = Random01(cx, cy, seed);
			const float b = Random01(cx + 1, cy, seed);
			const float c = Random01(cx, cy + 1, seed);
			const float d = Random01(cx + 1, cy + 1, seed);
			return std::lerp(std::lerp(a, b, fx), std::lerp(c, d, fx), fy);
		};

	const int soft = MakeXRGB8ColorSoftImage(width, height);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const float noise = valueNoise(static_cast<float>(x), static_cast<float>(y), kNoiseCellLarge, 31) * 0.7f
				+ valueNoise(static_cast<float>(x), static_cast<float>(y), kNoiseCellSmall, 57) * 0.3f;
			//ワイプは左ほど明るい(左から出る)
			const float value = isWipe ? (1.0f - static_cast<float>(x) / width) + (noise - 0.5f) * kWipeJitter : noise;
			const int v = std::clamp(static_cast<int>(value * 255.0f), 0, 255);
			DrawPixelSoftImage(soft, x, y, v, v, v, 0);
		}
	}
	const int graph = CreateGraphFromSoftImage(soft);
	DeleteSoftImage(soft);
	return graph;
}

float EndingUI::Random01(int a, int b, int c)
{
	unsigned int h = static_cast<unsigned int>(a) * 73856093u ^ static_cast<unsigned int>(b) * 19349663u ^ static_cast<unsigned int>(c) * 83492791u;
	h ^= h >> 13;
	h *= 0x5bd1e995u;
	h ^= h >> 15;
	return (h & 0xffffff) / static_cast<float>(0xffffff);
}
