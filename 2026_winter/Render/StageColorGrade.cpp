#include "StageColorGrade.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>

namespace
{
	struct Preset
	{
		COLOR_F dark;//暗い所の色
		COLOR_F bright;//明るい所の色
		float blackLevel;//これより暗いと dark
		float whiteLevel;//これより明るいと bright
		int backR, backG, backB;//背景の色
	};

	const Preset kRedPreset = { { 0.06f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.08f, 0.06f, 1.0f }, 0.02f, 0.55f, 120, 0, 0 };
	const Preset kGrayPreset = { { 0.02f, 0.02f, 0.02f, 1.0f }, { 0.8f, 0.8f, 0.8f, 1.0f }, 0.02f, 0.55f, 40, 40, 40 };

	//実フレームで進める
	constexpr float kFadeInFrame = 8.0f;
	constexpr float kFadeOutFrame = 15.0f;
}

void StageColorGrade::Update(bool isActive, Type type)
{
	if (isActive) m_type = type;
	const float add = isActive ? 1.0f / kFadeInFrame : -1.0f / kFadeOutFrame;
	m_rate = std::clamp(m_rate + add, 0.0f, 1.0f);
}

void StageColorGrade::DrawBackground() const
{
	if (m_rate <= 0.0f) return;

	const Preset& preset = m_type == Type::Red ? kRedPreset : kGrayPreset;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * m_rate));
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(preset.backR, preset.backG, preset.backB), true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void StageColorGrade::Begin() const
{
	if (m_rate <= 0.0f) return;

	const Preset& preset = m_type == Type::Red ? kRedPreset : kGrayPreset;
	ModelShaderManager::GetInstance().SetColorGrade(preset.dark, preset.bright, preset.blackLevel, preset.whiteLevel, m_rate);
}

void StageColorGrade::End() const
{
	if (m_rate <= 0.0f) return;

	ModelShaderManager::GetInstance().ClearColorGrade();
}
