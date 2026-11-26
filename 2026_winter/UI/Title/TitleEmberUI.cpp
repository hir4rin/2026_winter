#include "TitleEmberUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include <cmath>

namespace
{
	constexpr int kNum = 70;

	//設計解像度のピクセル
	constexpr float kSpeedMin = 1.2f;
	constexpr float kSpeedMax = 5.0f;//手前ほど速い
	constexpr float kRiseMax = 0.6f;
	constexpr float kSizeMin = 1.0f;
	constexpr float kSizeMax = 3.2f;
	constexpr float kWaveAmp = 0.5f;
	constexpr float kWaveSpeed = 0.05f;
	constexpr float kMargin = 40.0f;

	constexpr int kLifeMin = 180;
	constexpr int kLifeMax = 420;
	constexpr int kFadeFrame = 30;

	//斬った瞬間の突風
	constexpr float kGustPower = 3.0f;
	constexpr float kGustDecay = 0.95f;

	//尾を引く長さ(速さに対する倍率)
	constexpr float kTrail = 6.0f;
	constexpr float kGlowScale = 3.5f;
	constexpr int kGlowLayer = 4;//重ねて外ほど薄く
	constexpr float kGlowAlpha = 22.0f;
}

void TitleEmberUI::Init()
{
	m_embers.resize(kNum);
	for (auto& e : m_embers)Spawn(e, true);
	m_isVisible = true;
}

void TitleEmberUI::Spawn(Ember& e, bool anywhere)
{
	const float w = static_cast<float>(Game::kScreenWidth);
	const float h = static_cast<float>(Game::GetScreenHeight()) / Game::GetScale();
	auto rand01 = []() { return GetRand(10000) / 10000.0f; };

	e.depth = rand01();
	e.depth *= e.depth;//奥を多めに
	e.x = anywhere ? rand01() * w : w + rand01() * kMargin;
	e.y = rand01() * h;
	e.vx = -(kSpeedMin + (kSpeedMax - kSpeedMin) * e.depth) * (0.8f + 0.4f * rand01());
	e.vy = -kRiseMax * rand01();
	e.size = kSizeMin + (kSizeMax - kSizeMin) * e.depth;
	e.phase = rand01() * DX_TWO_PI_F;
	e.maxLife = kLifeMin + GetRand(kLifeMax - kLifeMin);
	e.life = anywhere ? GetRand(e.maxLife) : 0;
}

void TitleEmberUI::Update()
{
	m_frame++;
	m_gust *= kGustDecay;
	const float wind = 1.0f + m_gust;
	for (auto& e : m_embers)
	{
		e.life++;
		e.x += e.vx * wind;
		e.y += e.vy + std::sin(m_frame * kWaveSpeed + e.phase) * kWaveAmp;
		if (e.life >= e.maxLife || e.x < -kMargin || e.y < -kMargin)Spawn(e, false);
	}
}

void TitleEmberUI::Draw()
{
	if (!m_isVisible)return;
	const float scale = Game::GetScale();
	const float wind = 1.0f + m_gust;

	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
	for (const auto& e : m_embers)
	{
		//出始めと消える前は薄く
		float fade = 1.0f;
		if (e.life < kFadeFrame)fade = static_cast<float>(e.life) / kFadeFrame;
		const int rest = e.maxLife - e.life;
		if (rest < kFadeFrame)fade = static_cast<float>(rest) / kFadeFrame;
		//ちらつき
		const float flicker = 0.7f + 0.3f * std::sin(m_frame * 0.3f + e.phase * 3.0f);
		const float bright = fade * flicker * (0.45f + 0.55f * e.depth);

		const float x = e.x * scale;
		const float y = e.y * scale;
		const float r = e.size * scale;

		//外の光
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(kGlowAlpha * bright));
		for (int i = 0; i < kGlowLayer; i++)
		{
			const float rate = 1.0f - static_cast<float>(i) / kGlowLayer;
			DrawCircleAA(x, y, r * (1.0f + (kGlowScale - 1.0f) * rate), 16, GetColor(255, 60, 10), TRUE);
		}

		//尾(根元は太く明るく、先は細く薄く)
		const float tx = x - e.vx * wind * kTrail * scale;
		const float ty = y - e.vy * kTrail * scale;
		const float mx = (x + tx) * 0.5f;
		const float my = (y + ty) * 0.5f;
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(70 * bright));
		DrawLineAA(mx, my, tx, ty, GetColor(255, 70, 15), r * 0.5f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(160 * bright));
		DrawLineAA(x, y, mx, my, GetColor(255, 110, 30), r * 0.9f);

		//芯
		SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>(255 * bright));
		DrawCircleAA(x, y, r * 0.6f, 10, GetColor(255, 200, 120), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void TitleEmberUI::Gust()
{
	m_gust = kGustPower;
}
