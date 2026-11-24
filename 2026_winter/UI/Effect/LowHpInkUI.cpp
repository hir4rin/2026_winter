#include "LowHpInkUI.h"
#include "DxLib.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "Battle/BattleManager.h"
#include "Character/Player/Player.h"
#include "Render/ModelShaderManager.h"
#include <algorithm>
#include <cmath>

namespace
{
	const wchar_t* const kShaderPath = L"Render/Shader/LowHpInkPS.hlsl";
	constexpr int kConstantSlot = 10;//register(b10)

	//かかり方(HPの割合)
	constexpr float kStartHpRate = 0.30f;//これより下で効き始める
	constexpr float kMaxHpRate = 0.05f;//ここで一番強い

	//効き具合が目標へ寄る速さ(1フレーム)
	constexpr float kRateUpFollow = 0.08f;
	constexpr float kRateDownFollow = 0.04f;

	//心拍
	constexpr float kBeatMinBpm = 72.0f;
	constexpr float kBeatMaxBpm = 150.0f;
	constexpr float kFps = 60.0f;
}

LowHpInkUI::LowHpInkUI(std::weak_ptr<Player> player) :
	m_player(player)
{
}

LowHpInkUI::~LowHpInkUI()
{
	DeleteShader(m_shader);
	for (int handle : m_constantBuffers) DeleteShaderConstantBuffer(handle);
}

void LowHpInkUI::Init()
{
	m_shader = ModelShaderManager::LoadShaderWithMacro(kShaderPath, "ps_5_0", { {nullptr, nullptr} }, false);
	for (auto& handle : m_constantBuffers) handle = CreateShaderConstantBuffer(sizeof(InkConstant));
	m_rate = CalcTargetRate();
	m_isVisible = true;
}

void LowHpInkUI::Update()
{
	const float target = CalcTargetRate();
	const float follow = (target > m_rate) ? kRateUpFollow : kRateDownFollow;
	m_rate += (target - m_rate) * follow;
	if (target <= 0.0f && m_rate < 0.002f) m_rate = 0.0f;

	++m_frame;
	const float bpm = kBeatMinBpm + (kBeatMaxBpm - kBeatMinBpm) * m_rate;
	m_beatPhase = std::fmod(m_beatPhase + bpm / 60.0f / kFps, 1.0f);
	m_beat = BeatShape(m_beatPhase);
}

void LowHpInkUI::Draw()
{
	if (m_rate <= 0.0f || m_shader == -1) return;

	SetUsePixelShader(m_shader);
	SetDrawBlendMode(DX_BLENDMODE_MUL, 255);
	DrawPass(0);
	SetDrawBlendMode(DX_BLENDMODE_PMA_ADD, 255);
	DrawPass(1);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetUsePixelShader(-1);
}

float LowHpInkUI::CalcTargetRate() const
{
	auto player = m_player.lock();
	if (!player) return 0.0f;
	auto battle = System::GetInstance().GetBattleMgr();
	if (battle && (battle->GetFinisherScreen().isPlaying || battle->GetEndingScreen().isPlaying || battle->GetIsEventPlaying())) return 0.0f;

	const int hp = player->GetHp();
	//死亡演出とは重ねない
	if (hp <= 0) return 0.0f;
	const float hpRate = static_cast<float>(hp) / player->GetMaxHp();
	return std::clamp((kStartHpRate - hpRate) / (kStartHpRate - kMaxHpRate), 0.0f, 1.0f);
}

float LowHpInkUI::BeatShape(float phase)
{
	auto bump = [](float x, float center, float width) { const float d = (x - center) / width; return std::exp(-d * d); };
	return (std::min)(1.0f, bump(phase, 0.06f, 0.05f) + 0.65f * bump(phase, 0.27f, 0.06f));
}

void LowHpInkUI::DrawPass(int pass)
{
	const int handle = m_constantBuffers[pass];
	const float w = static_cast<float>(Game::GetScreenWidth());
	const float h = static_cast<float>(Game::GetScreenHeight());

	auto* c = static_cast<InkConstant*>(GetBufferShaderConstantBuffer(handle));
	c->param[0] = m_rate;
	c->param[1] = m_beat;
	c->param[2] = m_frame / kFps;
	c->param[3] = static_cast<float>(pass);
	c->screen[0] = w / h;
	c->screen[1] = c->screen[2] = c->screen[3] = 0.0f;
	UpdateShaderConstantBuffer(handle);
	SetShaderConstantBuffer(handle, DX_SHADERTYPE_PIXEL, kConstantSlot);

	VERTEX2DSHADER v[4] = {};
	const float pos[4][4] = { {0.0f, 0.0f, 0.0f, 0.0f}, {w, 0.0f, 1.0f, 0.0f}, {0.0f, h, 0.0f, 1.0f}, {w, h, 1.0f, 1.0f} };
	for (int i = 0; i < 4; ++i)
	{
		v[i].pos = VGet(pos[i][0], pos[i][1], 0.0f);
		v[i].rhw = 1.0f;
		v[i].dif = GetColorU8(255, 255, 255, 255);
		v[i].spc = GetColorU8(0, 0, 0, 0);
		v[i].u = v[i].su = pos[i][2];
		v[i].v = v[i].sv = pos[i][3];
	}
	DrawPrimitive2DToShader(v, 4, DX_PRIMTYPE_TRIANGLESTRIP);
}
