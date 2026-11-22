#include "SwordLook.h"
#include "Render/ModelShaderManager.h"
#include "Render/CharacterLook.h"
#include <algorithm>
#ifdef _DEBUG
#include "imgui.h"
#endif

namespace
{
	constexpr float kEdgeEmissiveMin = 1.0f;//マテリアルの発光(赤)がこれより強いメッシュを刃側とする
}

SwordLook::LookSettings& SwordLook::Settings()
{
	static LookSettings settings;
	return settings;
}

SwordLook::UltLookSettings& SwordLook::UltSettings()
{
	static UltLookSettings settings;
	return settings;
}

SwordLook::~SwordLook()
{
	Release();
}

int SwordLook::GetMaterialGraph(int modelHandle, int textureIndex)
{
	if (textureIndex < 0) return -1;
	return MV1GetTextureGraphHandle(modelHandle, textureIndex);
}

bool SwordLook::Load(int modelHandle)
{
	Release();
	m_isUlt = false;
	if (modelHandle < 0 || MV1GetMaterialNum(modelHandle) <= 0) return false;
	//katana_blend はマテリアル1つ。法線マップとマスク(スペキュラの画像)を持っている
	m_normalTex = GetMaterialGraph(modelHandle, MV1GetMaterialNormalMapTexture(modelHandle, 0));
	m_maskTex = GetMaterialGraph(modelHandle, MV1GetMaterialSpcMapTexture(modelHandle, 0));
	if (m_normalTex == -1 || m_maskTex == -1) return false;
	m_constantBuffer = CreateShaderConstantBuffer(sizeof(SwordConstant));
	m_isLoaded = m_constantBuffer != -1;
	return m_isLoaded;
}

bool SwordLook::LoadUlt(int modelHandle, const std::vector<bool>& isBladeMesh)
{
	Release();
	m_isUlt = true;
	if (modelHandle < 0) return false;
	//刀身のメッシュのうち、赤く光るマテリアルが刃側、それ以外が峰側
	const int meshNum = MV1GetMeshNum(modelHandle);
	m_meshParts.assign(meshNum, Part::Fitting);
	bool hasEdge = false;
	for (int mesh = 0; mesh < meshNum && mesh < static_cast<int>(isBladeMesh.size()); ++mesh)
	{
		if (!isBladeMesh[mesh]) continue;
		const COLOR_F emi = MV1GetMaterialEmiColor(modelHandle, MV1GetMeshMaterial(modelHandle, mesh));
		const bool isEdge = emi.r > kEdgeEmissiveMin;
		m_meshParts[mesh] = isEdge ? Part::Edge : Part::Core;
		hasEdge |= isEdge;
	}
	if (!hasEdge) return false;
	m_constantBuffer = CreateShaderConstantBuffer(sizeof(UltConstant));
	m_isLoaded = m_constantBuffer != -1;
	return m_isLoaded;
}

void SwordLook::Update(bool isSlashing)
{
	const LookSettings& s = Settings();
	//斬りの光は出るのは速く、消えるのはゆっくり
	m_slashRate = isSlashing ? (std::min)(m_slashRate + s.slashRise, 1.0f) : (std::max)(m_slashRate - s.slashFall, 0.0f);
	m_frame += 1.0f;
}

bool SwordLook::IsEnable() const
{
	return m_isUlt ? UltSettings().isEnable : Settings().isEnable;
}

int SwordLook::GetPixelShader(bool isBump) const
{
	auto& shaderManager = ModelShaderManager::GetInstance();
	return m_isUlt ? shaderManager.GetUltSwordPixelShader(isBump) : shaderManager.GetSwordPixelShader(isBump);
}

void SwordLook::Draw(int modelHandle)
{
	auto& shaderManager = ModelShaderManager::GetInstance();
	//血殺中(黒くするモード)・読めなかったときは今まで通り
	if (!m_isLoaded || !IsEnable() || !shaderManager.IsInit() || shaderManager.GetBlackMode() || GetPixelShader(false) == -1)
	{
		shaderManager.DrawModel(modelHandle);
		return;
	}
	if (!m_isUlt)
	{
		SetConstant();
		SetUseTextureToShader(1, m_normalTex);
		SetUseTextureToShader(2, m_maskTex);
	}
	shaderManager.BindStageLights();

	const int meshNum = MV1GetMeshNum(modelHandle);
	for (int mesh = 0; mesh < meshNum; ++mesh)
	{
		if (MV1GetMeshTListNum(modelHandle, mesh) <= 0) continue;
		if (!MV1GetMeshVisible(modelHandle, mesh)) continue;
		const int type = MV1GetTriangleListVertexType(modelHandle, MV1GetMeshTList(modelHandle, mesh, 0));
		const int vs = shaderManager.GetVertexShader(type);
		if (vs == -1)
		{
			MV1SetUseOrigShader(FALSE);
			MV1DrawMesh(modelHandle, mesh);
			continue;
		}
		//長い刀はメッシュごとに部品を渡す
		if (m_isUlt) SetUltConstant(modelHandle, mesh < static_cast<int>(m_meshParts.size()) ? m_meshParts[mesh] : Part::Fitting);
		const bool isBump = type == DX_MV1_VERTEX_TYPE_NMAP_1FRAME || type == DX_MV1_VERTEX_TYPE_NMAP_4FRAME || type == DX_MV1_VERTEX_TYPE_NMAP_8FRAME;
		MV1SetUseOrigShader(TRUE);
		SetUseVertexShader(vs);
		SetUsePixelShader(GetPixelShader(isBump));
		MV1DrawMesh(modelHandle, mesh);
	}

	//元に戻す
	SetUseTextureToShader(1, -1);
	SetUseTextureToShader(2, -1);
	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	MV1SetUseOrigShader(FALSE);
}

void SwordLook::SetConstant()
{
	const LookSettings& s = Settings();
	const CharacterLook::LookSettings& light = CharacterLook::Settings();
	auto* c = static_cast<SwordConstant*>(GetBufferShaderConstantBuffer(m_constantBuffer));
	const VECTOR dir = VNorm(light.keyDir);
	const VECTOR camera = GetCameraPosition();
	c->cameraPos = { camera.x, camera.y, camera.z, m_frame };
	c->keyDir = { dir.x, dir.y, dir.z, m_slashRate };
	c->keyColor = { light.keyColor.r, light.keyColor.g, light.keyColor.b, s.sweepPeriod };
	c->skyColor = { light.skyColor.r, light.skyColor.g, light.skyColor.b, s.envStrength };
	c->groundColor = { light.groundColor.r, light.groundColor.g, light.groundColor.b, s.rimStrength };
	c->envSky = { s.envSky.r, s.envSky.g, s.envSky.b, 0.0f };
	c->envGround = { s.envGround.r, s.envGround.g, s.envGround.b, 0.0f };
	c->envHorizon = { s.envHorizon.r, s.envHorizon.g, s.envHorizon.b, 0.0f };
	c->envStrip = { s.envStrip.r, s.envStrip.g, s.envStrip.b, 0.0f };
	c->steel = { s.steel.r, s.steel.g, s.steel.b, s.steelRough };
	c->hamon = { s.hamon.r, s.hamon.g, s.hamon.b, s.hamonLine };
	c->edge = { s.edge.r, s.edge.g, s.edge.b, s.edgeIntensity };
	c->sweep = { s.sweep.r, s.sweep.g, s.sweep.b, s.sweepWidth };
	c->param = { s.edgeSlash, light.normalFlipY, s.edgeWidth, s.slashEnvBoost };
	c->blade = { s.bladeCenterU, s.bladeHalfU, s.bladeBaseV, s.bladeTipV };
	UpdateShaderConstantBuffer(m_constantBuffer);
	SetShaderConstantBuffer(m_constantBuffer, DX_SHADERTYPE_PIXEL, kSwordSlot);
}

void SwordLook::SetUltConstant(int modelHandle, Part part)
{
	const UltLookSettings& s = UltSettings();
	const CharacterLook::LookSettings& light = CharacterLook::Settings();
	auto* c = static_cast<UltConstant*>(GetBufferShaderConstantBuffer(m_constantBuffer));
	const VECTOR dir = VNorm(light.keyDir);
	const VECTOR camera = GetCameraPosition();
	c->cameraPos = { camera.x, camera.y, camera.z, m_frame };
	c->keyDir = { dir.x, dir.y, dir.z, m_slashRate };
	c->keyColor = { light.keyColor.r, light.keyColor.g, light.keyColor.b, static_cast<float>(part) };
	c->skyColor = { light.skyColor.r, light.skyColor.g, light.skyColor.b, s.envStrength };
	c->groundColor = { light.groundColor.r, light.groundColor.g, light.groundColor.b, s.rimStrength };
	c->envSky = { s.envSky.r, s.envSky.g, s.envSky.b, 0.0f };
	c->envHorizon = { s.envHorizon.r, s.envHorizon.g, s.envHorizon.b, 0.0f };
	c->envStrip = { s.envStrip.r, s.envStrip.g, s.envStrip.b, 0.0f };
	c->core = { s.core.r, s.core.g, s.core.b, s.coreRough };
	c->bloodDeep = { s.bloodDeep.r, s.bloodDeep.g, s.bloodDeep.b, s.flowSpeed };
	c->bloodBright = { s.bloodBright.r, s.bloodBright.g, s.bloodBright.b, s.pulsePeriod };
	c->edgeHot = { s.edgeHot.r, s.edgeHot.g, s.edgeHot.b, s.veinIntensity };
	c->vein = { s.vein.r, s.vein.g, s.vein.b, s.slashBoost };
	c->shape = { s.tipZ, s.baseZ, s.borderY, s.edgeY };
	c->shape2 = { s.spineY, 0.0f, 0.0f, 0.0f };
	//ワールド → 刀のフレームの空間(伸ばして描くときも、伸ばす前の空間に戻る)
	const MATRIX toLocal = MInverse(MV1GetFrameLocalWorldMatrix(modelHandle, 0));
	for (int row = 0; row < 4; ++row) c->toLocal[row] = { toLocal.m[row][0], toLocal.m[row][1], toLocal.m[row][2], toLocal.m[row][3] };
	UpdateShaderConstantBuffer(m_constantBuffer);
	SetShaderConstantBuffer(m_constantBuffer, DX_SHADERTYPE_PIXEL, kUltSlot);
}

void SwordLook::DrawDebugWindow()
{
#ifdef _DEBUG
	LookSettings& s = Settings();
	ImGui::Begin("Sword Look");
	ImGui::Checkbox("Enable", &s.isEnable);
	ImGui::SeparatorText("Reflection");
	ImGui::ColorEdit3("Env Sky", &s.envSky.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Env Ground", &s.envGround.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Env Horizon", &s.envHorizon.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Env Strip", &s.envStrip.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::SliderFloat("Env Strength", &s.envStrength, 0.0f, 3.0f);
	ImGui::SeparatorText("Blade");
	ImGui::ColorEdit3("Steel", &s.steel.r, ImGuiColorEditFlags_Float);
	ImGui::SliderFloat("Steel Rough", &s.steelRough, 0.02f, 0.6f);
	ImGui::ColorEdit3("Hamon", &s.hamon.r, ImGuiColorEditFlags_Float);
	ImGui::SliderFloat("Hamon Line", &s.hamonLine, 0.0f, 0.6f);
	ImGui::ColorEdit3("Edge", &s.edge.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::SliderFloat("Edge Intensity", &s.edgeIntensity, 0.0f, 3.0f);
	ImGui::SliderFloat("Edge Width", &s.edgeWidth, 0.01f, 0.3f);
	ImGui::SliderFloat("Edge Slash", &s.edgeSlash, 0.0f, 6.0f);
	ImGui::ColorEdit3("Sweep", &s.sweep.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::SliderFloat("Sweep Width", &s.sweepWidth, 0.01f, 0.3f);
	ImGui::SliderFloat("Sweep Period", &s.sweepPeriod, 20.0f, 600.0f);
	ImGui::SliderFloat("Rim", &s.rimStrength, 0.0f, 2.0f);
	ImGui::SliderFloat("Slash Env Boost", &s.slashEnvBoost, 0.0f, 3.0f);

	UltLookSettings& u = UltSettings();
	ImGui::SeparatorText("Ult Sword");
	ImGui::Checkbox("Ult Enable", &u.isEnable);
	ImGui::ColorEdit3("Ult Env Sky", &u.envSky.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Ult Env Horizon", &u.envHorizon.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Ult Env Strip", &u.envStrip.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::SliderFloat("Ult Env Strength", &u.envStrength, 0.0f, 3.0f);
	ImGui::ColorEdit3("Core", &u.core.r, ImGuiColorEditFlags_Float);
	ImGui::SliderFloat("Core Rough", &u.coreRough, 0.02f, 0.6f);
	ImGui::ColorEdit3("Blood Deep", &u.bloodDeep.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Blood Bright", &u.bloodBright.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Edge Hot", &u.edgeHot.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Vein", &u.vein.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::SliderFloat("Vein Intensity", &u.veinIntensity, 0.0f, 3.0f);
	ImGui::SliderFloat("Flow Speed", &u.flowSpeed, 0.0f, 0.1f);
	ImGui::SliderFloat("Pulse Period", &u.pulsePeriod, 10.0f, 200.0f);
	ImGui::SliderFloat("Ult Rim", &u.rimStrength, 0.0f, 2.0f);
	ImGui::SliderFloat("Slash Boost", &u.slashBoost, 0.0f, 3.0f);
	ImGui::End();
#endif
}

void SwordLook::Release()
{
	m_normalTex = -1;
	m_maskTex = -1;
	m_meshParts.clear();
	if (m_constantBuffer != -1) DeleteShaderConstantBuffer(m_constantBuffer);
	m_constantBuffer = -1;
	m_isLoaded = false;
}
