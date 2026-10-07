#include "BloodKillFog.h"
#include "DxLib.h"
#include "../Game.h"
#include <algorithm>

namespace
{
	//背景とフォグの色(同じ色にすると、ステージの奥が背景と境目なくつながる)
	constexpr int kRed = 120;
	constexpr int kGreen = 0;
	constexpr int kBlue = 0;

	//フォグがかかり始める距離と、完全に赤になる距離
	constexpr float kFogStart = 300.0f;
	constexpr float kFogEnd = 2000.0f;
	//フォグがかかっていないときの距離(カメラのFar以上にしておく)
	constexpr float kNoFogDistance = 10000.0f;

	//ステージのカラースケール(暗い赤にして、キャラとの明るさの差をつける)
	const COLOR_F kStageColorScale = { 0.5f, 0.15f, 0.15f, 1.0f };

	//フェードにかけるフレーム数(タイムスケールの影響を受けないよう実フレームで進める)
	constexpr float kFadeInFrame = 8.0f;
	constexpr float kFadeOutFrame = 15.0f;

	float Lerp(float a, float b, float t) { return a + (b - a) * t; }
}

void BloodKillFog::Update(bool isActive)
{
#if BLOOD_KILL_FOG_ENABLE
	const float add = isActive ? 1.0f / kFadeInFrame : -1.0f / kFadeOutFrame;
	m_rate = std::clamp(m_rate + add, 0.0f, 1.0f);
#endif
}

void BloodKillFog::DrawBackground() const
{
#if BLOOD_KILL_FOG_ENABLE
	if (m_rate <= 0.0f) return;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * m_rate));
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(kRed, kGreen, kBlue), true);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
#endif
}

void BloodKillFog::BeginStage(int stageViewHandle) const
{
#if BLOOD_KILL_FOG_ENABLE
	if (m_rate <= 0.0f) return;

	SetFogEnable(TRUE);
	SetFogMode(DX_FOGMODE_LINEAR);
	SetFogColor(kRed, kGreen, kBlue);
	//かかり具合に合わせて、遠くからだんだん手前までフォグを寄せる
	SetFogStartEnd(Lerp(kNoFogDistance, kFogStart, m_rate), Lerp(kNoFogDistance, kFogEnd, m_rate));

	const COLOR_F scale = {
		Lerp(1.0f, kStageColorScale.r, m_rate),
		Lerp(1.0f, kStageColorScale.g, m_rate),
		Lerp(1.0f, kStageColorScale.b, m_rate),
		1.0f };
	MV1SetDifColorScale(stageViewHandle, scale);
#endif
}

void BloodKillFog::EndStage(int stageViewHandle) const
{
#if BLOOD_KILL_FOG_ENABLE
	if (m_rate <= 0.0f) return;

	//キャラやエフェクトにフォグがかからないように戻す
	SetFogEnable(FALSE);
	MV1SetDifColorScale(stageViewHandle, GetColorF(1.0f, 1.0f, 1.0f, 1.0f));
#endif
}
