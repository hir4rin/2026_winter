#include "CharacterLook.h"
#include "Render/ModelShaderManager.h"
#include <fstream>
#include <map>
#include <sstream>
#ifdef _DEBUG
#include "imgui.h"
#endif

namespace
{
	//CSV の列
	enum LookColumn
	{
		ColMaterial, ColNormalTex, ColMaskTex, ColNormalStrength, ColAoStrength, ColSmoothStrength, ColMetalStrength,
		ColSpecStrength, ColAlphaClip, ColTintR, ColTintG, ColTintB, ColDrawLast, ColNum
	};
}

CharacterLook::LookSettings& CharacterLook::Settings()
{
	static LookSettings settings;
	return settings;
}

CharacterLook::~CharacterLook()
{
	Release();
}

bool CharacterLook::Load(int modelHandle, const std::string& csvPath)
{
	Release();
	std::ifstream file(csvPath);
	if (!file.is_open() || modelHandle < 0) return false;
	const std::string dir = csvPath.substr(0, csvPath.find_last_of("/\\") + 1);

	auto loadGraph = [&](const std::string& name) { return LoadCachedGraph(dir, name); };

	//マテリアル名の先頭が一致する行を、そのマテリアルに当てる
	m_materials.assign(MV1GetMaterialNum(modelHandle), MaterialLook());
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty() || line[0] == '#') continue;
		std::vector<std::string> tokens;
		std::istringstream ss(line);
		std::string token;
		while (std::getline(ss, token, ',')) tokens.push_back(token);
		if (tokens.size() < ColNum) continue;

		MaterialLook look;
		try
		{
			look.normalTex = loadGraph(tokens[ColNormalTex]);
			look.maskTex = loadGraph(tokens[ColMaskTex]);
			look.normalStrength = std::stof(tokens[ColNormalStrength]);
			look.aoStrength = std::stof(tokens[ColAoStrength]);
			look.smoothStrength = std::stof(tokens[ColSmoothStrength]);
			look.metalStrength = std::stof(tokens[ColMetalStrength]);
			look.specStrength = std::stof(tokens[ColSpecStrength]);
			look.alphaClip = std::stof(tokens[ColAlphaClip]);
			look.tint[0] = std::stof(tokens[ColTintR]);
			look.tint[1] = std::stof(tokens[ColTintG]);
			look.tint[2] = std::stof(tokens[ColTintB]);
			look.drawLast = std::stoi(tokens[ColDrawLast]) != 0;
		}
		catch (...)
		{
			continue;//数値にできない行は飛ばす
		}
		const std::string& prefix = tokens[ColMaterial];
		for (int i = 0; i < static_cast<int>(m_materials.size()); ++i)
		{
			if (std::string(MV1GetMaterialName(modelHandle, i)).rfind(prefix, 0) == 0) m_materials[i] = look;
		}
	}
	m_constantBuffer = CreateShaderConstantBuffer(sizeof(LookConstant));
	m_isLoaded = m_constantBuffer != -1;
	return m_isLoaded;
}

void CharacterLook::Draw(int modelHandle)
{
	auto& shaderManager = ModelShaderManager::GetInstance();
	const LookSettings& settings = Settings();
	if (!m_isLoaded || !settings.isEnable || !shaderManager.IsInit() || shaderManager.GetLookPixelShader(false) == -1)
	{
		shaderManager.DrawModel(modelHandle);//今まで通り(ステージの明かりは重ね描き)
		return;
	}
	shaderManager.BindStageLights();

	//不透明なものを先に、半透明(髪・まつげ)を後に描く
	const int meshNum = MV1GetMeshNum(modelHandle);
	for (int pass = 0; pass < 2; ++pass)
	{
		for (int mesh = 0; mesh < meshNum; ++mesh)
		{
			if (MV1GetMeshTListNum(modelHandle, mesh) <= 0) continue;
			const int material = MV1GetMeshMaterial(modelHandle, mesh);
			if (material < 0 || material >= static_cast<int>(m_materials.size())) continue;
			const MaterialLook& look = m_materials[material];
			if (look.drawLast != (pass == 1)) continue;
			const int type = MV1GetTriangleListVertexType(modelHandle, MV1GetMeshTList(modelHandle, mesh, 0));
			const int vs = shaderManager.GetVertexShader(type);
			if (vs == -1)
			{
				//頂点シェーダーの無い頂点タイプは標準の描き方
				MV1SetUseOrigShader(FALSE);
				MV1DrawMesh(modelHandle, mesh);
				continue;
			}
			const bool isBump = type == DX_MV1_VERTEX_TYPE_NMAP_1FRAME || type == DX_MV1_VERTEX_TYPE_NMAP_4FRAME || type == DX_MV1_VERTEX_TYPE_NMAP_8FRAME;
			SetMaterialConstant(look);
			SetUseTextureToShader(1, look.normalTex);
			SetUseTextureToShader(2, look.maskTex);
			MV1SetUseOrigShader(TRUE);
			SetUseVertexShader(vs);
			SetUsePixelShader(shaderManager.GetLookPixelShader(isBump));
			MV1DrawMesh(modelHandle, mesh);
		}
	}

	//元に戻す(ほかの描画に影響させないため)
	SetUseTextureToShader(1, -1);
	SetUseTextureToShader(2, -1);
	SetUseVertexShader(-1);
	SetUsePixelShader(-1);
	MV1SetUseOrigShader(FALSE);
}

void CharacterLook::SetMaterialConstant(const MaterialLook& look)
{
	const LookSettings& s = Settings();
	auto* c = static_cast<LookConstant*>(GetBufferShaderConstantBuffer(m_constantBuffer));
	const VECTOR dir = VNorm(s.keyDir);
	const VECTOR camera = GetCameraPosition();
	c->keyDir = { dir.x, dir.y, dir.z, 0.0f };
	c->keyColor = { s.keyColor.r, s.keyColor.g, s.keyColor.b, 0.0f };
	c->skyColor = { s.skyColor.r, s.skyColor.g, s.skyColor.b, 0.0f };
	c->groundColor = { s.groundColor.r, s.groundColor.g, s.groundColor.b, 0.0f };
	c->cameraPos = { camera.x, camera.y, camera.z, 0.0f };
	c->rim = { s.rimColor.r, s.rimColor.g, s.rimColor.b, s.rimPower };
	c->mat0 = { look.normalStrength, look.aoStrength, look.smoothStrength, look.metalStrength };
	c->mat1 = { look.normalTex != -1 ? 1.0f : 0.0f, look.maskTex != -1 ? 1.0f : 0.0f, look.alphaClip, look.specStrength };
	c->tint = { look.tint[0], look.tint[1], look.tint[2], s.normalFlipY };
	UpdateShaderConstantBuffer(m_constantBuffer);
	SetShaderConstantBuffer(m_constantBuffer, DX_SHADERTYPE_PIXEL, kLookSlot);
}

void CharacterLook::DrawDebugWindow()
{
#ifdef _DEBUG
	LookSettings& s = Settings();
	ImGui::Begin("Character Look");
	ImGui::Checkbox("Enable", &s.isEnable);
	ImGui::DragFloat3("Key Dir", &s.keyDir.x, 0.02f, -1.0f, 1.0f);
	ImGui::ColorEdit3("Key Color", &s.keyColor.r, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
	ImGui::ColorEdit3("Sky Color", &s.skyColor.r, ImGuiColorEditFlags_Float);
	ImGui::ColorEdit3("Ground Color", &s.groundColor.r, ImGuiColorEditFlags_Float);
	ImGui::ColorEdit3("Rim Color", &s.rimColor.r, ImGuiColorEditFlags_Float);
	ImGui::SliderFloat("Rim Power", &s.rimPower, 0.5f, 8.0f);
	bool flip = s.normalFlipY < 0.0f;
	if (ImGui::Checkbox("Flip Normal Y", &flip)) s.normalFlipY = flip ? -1.0f : 1.0f;
	ImGui::End();
#endif
}

std::map<std::string, int>& CharacterLook::GraphCache()
{
	static std::map<std::string, int> cache;
	return cache;
}

int CharacterLook::LoadCachedGraph(const std::string& dir, const std::string& name)
{
	//同じ画像は1回だけ読む(前のシーンで読んだものも使う)
	if (name.empty() || name == "-") return -1;
	const std::string path = dir + name;
	auto& graphs = GraphCache();
	auto it = graphs.find(path);
	if (it != graphs.end()) return it->second;
	const int handle = LoadGraph(path.c_str());
	graphs[path] = handle;
	return handle;
}

void CharacterLook::PreloadGraphs(const std::string& csvPath)
{
	std::ifstream file(csvPath);
	if (!file.is_open()) return;
	const std::string dir = csvPath.substr(0, csvPath.find_last_of("/\\") + 1);
	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty() || line[0] == '#') continue;
		std::vector<std::string> tokens;
		std::istringstream ss(line);
		std::string token;
		while (std::getline(ss, token, ',')) tokens.push_back(token);
		if (tokens.size() < ColNum) continue;
		LoadCachedGraph(dir, tokens[ColNormalTex]);
		LoadCachedGraph(dir, tokens[ColMaskTex]);
	}
}

void CharacterLook::ReleaseGraphCache()
{
	for (const auto& [path, handle] : GraphCache())
	{
		if (handle != -1) DeleteGraph(handle);
	}
	GraphCache().clear();
}

void CharacterLook::Release()
{
	//画像は使い回すのでここでは消さない(ReleaseGraphCache)
	m_materials.clear();
	if (m_constantBuffer != -1) DeleteShaderConstantBuffer(m_constantBuffer);
	m_constantBuffer = -1;
	m_isLoaded = false;
}
