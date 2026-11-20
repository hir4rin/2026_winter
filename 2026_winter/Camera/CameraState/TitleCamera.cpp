#include "TitleCamera.h"
#include "Main/Game.h"
#include <cmath>

namespace
{
	constexpr float kFov = 22.0f * DX_PI_F / 180.0f;//顔のアップがゆがまないように望遠
	//1単位=1ピクセルになる距離
	const float kDistance = Game::kScreenHeight * 0.5f / std::tan(kFov * 0.5f);
	constexpr float kIntroScale = 1.06f;//出始めの寄り
	constexpr int kIntroFrame = 45;
	//注視点はずっと奥//カメラシェイクで位置が動いてもほぼ平行に動くので、ロゴが揺れて見える
	constexpr float kLookAhead = 100000.0f;
}

TitleCamera::TitleCamera(std::weak_ptr<CameraManager> owner) :CameraStateBase(owner)
{
}

TitleCamera::~TitleCamera()
{
}

void TitleCamera::Enter(CameraData data)
{
	m_frame = 0;
	m_fov = kFov;
	m_goalFov = kFov;
	UpdatePos();
	ResetBlend(m_pos, m_target);
}

void TitleCamera::Update()
{
	if (m_frame < kIntroFrame)m_frame++;
	UpdatePos();
}

void TitleCamera::Exit()
{
}

void TitleCamera::SkipIntro()
{
	m_frame = kIntroFrame;
	UpdatePos();
}

void TitleCamera::UpdatePos()
{
	const float t = static_cast<float>(m_frame) / kIntroFrame;
	const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	const float scale = kIntroScale + (1.0f - kIntroScale) * ease;
	m_target = Vector3(0.0f, 0.0f, kLookAhead);
	m_pos = Vector3(0.0f, 0.0f, -kDistance / scale);
	m_goalPos = m_pos;
	m_goalTarget = m_target;
}
