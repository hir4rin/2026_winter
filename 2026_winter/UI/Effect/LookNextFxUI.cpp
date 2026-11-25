#include "LookNextFxUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Camera/CameraManager.h"
#include "Camera/CameraState/CameraStateBase.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>

namespace
{
	const wchar_t* const kShaderPath = L"Render/Shader/LookNextFxPS.hlsl";
	const char* const kInkPath = "data/UI/bloodUI/ink_vignette.png";
	constexpr int kConstantSlot = 11;//register(b11)

	constexpr float kInFrame = 10.0f;//かかりきるまで
	constexpr float kOutFrame = 15.0f;//消えるまで
	constexpr float kPulseFrame = 12.0f;//入りの放射ブラーの長さ
}

LookNextFxUI::LookNextFxUI(std::weak_ptr<CameraManager> cameraManager) :
	m_cameraManager(cameraManager)
{
}

LookNextFxUI::~LookNextFxUI()
{
	DeleteShader(m_shader);
	DeleteShaderConstantBuffer(m_constantBuffer);
	DeleteGraph(m_screen);
	DeleteGraph(m_inkHandle);
}

void LookNextFxUI::Init()
{
	m_shader = ModelShaderManager::LoadShaderWithMacro(kShaderPath, "ps_5_0", { {nullptr, nullptr} }, false);
	m_constantBuffer = CreateShaderConstantBuffer(sizeof(FxConstant));
	m_screen = MakeScreen(Game::GetScreenWidth(), Game::GetScreenHeight(), FALSE);
	m_inkHandle = LoadGraph(kInkPath);
	m_isVisible = true;
}

void LookNextFxUI::Update()
{
	const bool isLooking = IsLooking() && !IsBlocked();
	if (isLooking && !m_wasLooking) m_pulseFrame = 0;
	m_wasLooking = isLooking;

	if (isLooking) m_rate = (std::min)(1.0f, m_rate + 1.0f / kInFrame);
	else m_rate = (std::max)(0.0f, m_rate - 1.0f / kOutFrame);

	if (m_pulseFrame >= 0)
	{
		m_pulseFrame++;
		if (!isLooking || m_pulseFrame >= kPulseFrame) m_pulseFrame = -1;
	}
}

void LookNextFxUI::Draw()
{
	if (m_rate <= 0.0f || m_shader == -1 || m_screen == -1) return;

	//離して消えている間は前の位置のまま
	float u = 0.0f, v = 0.0f;
	if (IsLooking() && CalcTargetUv(u, v))
	{
		m_targetU = u;
		m_targetV = v;
	}

	const float w = static_cast<float>(Game::GetScreenWidth());
	const float h = static_cast<float>(Game::GetScreenHeight());
	GetDrawScreenGraph(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), m_screen);

	const float eased = 1.0f - (1.0f - m_rate) * (1.0f - m_rate) * (1.0f - m_rate);
	const float pulse = (m_pulseFrame >= 0) ? 1.0f - m_pulseFrame / kPulseFrame : 0.0f;

	auto* c = static_cast<FxConstant*>(GetBufferShaderConstantBuffer(m_constantBuffer));
	c->param[0] = eased;
	c->param[1] = pulse * pulse;
	c->param[2] = w / h;
	c->param[3] = 0.0f;
	c->target[0] = m_targetU;
	c->target[1] = m_targetV;
	c->target[2] = c->target[3] = 0.0f;
	UpdateShaderConstantBuffer(m_constantBuffer);
	SetShaderConstantBuffer(m_constantBuffer, DX_SHADERTYPE_PIXEL, kConstantSlot);

	VERTEX2DSHADER vtx[4] = {};
	const float pos[4][4] = { {0.0f, 0.0f, 0.0f, 0.0f}, {w, 0.0f, 1.0f, 0.0f}, {0.0f, h, 0.0f, 1.0f}, {w, h, 1.0f, 1.0f} };
	for (int i = 0; i < 4; ++i)
	{
		vtx[i].pos = VGet(pos[i][0], pos[i][1], 0.0f);
		vtx[i].rhw = 1.0f;
		vtx[i].dif = GetColorU8(255, 255, 255, 255);
		vtx[i].spc = GetColorU8(0, 0, 0, 0);
		vtx[i].u = vtx[i].su = pos[i][2];
		vtx[i].v = vtx[i].sv = pos[i][3];
	}

	SetUsePixelShader(m_shader);
	SetUseTextureToShader(0, m_screen);
	SetUseTextureToShader(1, m_inkHandle);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawPrimitive2DToShader(vtx, 4, DX_PRIMTYPE_TRIANGLESTRIP);
	SetUseTextureToShader(0, -1);
	SetUseTextureToShader(1, -1);
	SetUsePixelShader(-1);
}

bool LookNextFxUI::IsLooking() const
{
	auto cameraManager = m_cameraManager.lock();
	if (!cameraManager) return false;
	auto camera = cameraManager->GetActiveCamera();
	return camera && camera->GetCameraType() == CameraStateBase::Type::LookingNextCamera;
}

bool LookNextFxUI::IsBlocked() const
{
	auto battle = System::GetInstance().GetBattleMgr();
	return battle && (battle->GetFinisherScreen().isPlaying || battle->GetEndingScreen().isPlaying || battle->GetIsEventPlaying());
}

bool LookNextFxUI::CalcTargetUv(float& u, float& v) const
{
	auto cameraManager = m_cameraManager.lock();
	if (!cameraManager) return false;
	auto camera = cameraManager->GetActiveCamera();
	if (!camera) return false;

	const VECTOR target = camera->GetTarget().ToDxLibVector();
	if (VDot(VSub(target, GetCameraPosition()), GetCameraFrontVector()) <= 0.0f) return false;

	const VECTOR screenPos = ConvWorldPosToScreenPos(target);
	u = screenPos.x / Game::GetScreenWidth();
	v = screenPos.y / Game::GetScreenHeight();
	return true;
}
