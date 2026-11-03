#include "StageLights.h"
#include "Render/ModelShaderManager.h"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
	constexpr int kColumnNum = 9;//kind,posX,posY,posZ,colorR,colorG,colorB,range,intensity
}

StageLights::~StageLights()
{
	Release();
}

void StageLights::Load(int stageNumber, const Vector3& offset)
{
	Release();
	char path[64];
	snprintf(path, sizeof(path), "data/Stage/Lights/Stage%03d.csv", stageNumber);
	std::ifstream file(path);
	if (!file.is_open()) return;//明かりの無いステージ

	std::string line;
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (line.empty() || line[0] == '#') continue;

		std::istringstream ss(line);
		std::string token;
		std::vector<std::string> tokens;
		while (std::getline(ss, token, ','))
		{
			tokens.push_back(token);
		}
		if (tokens.size() < kColumnNum) continue;

		LightData data;
		try
		{
			data.pos = Vector3(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3])) + offset;
			data.r = std::stof(tokens[4]);
			data.g = std::stof(tokens[5]);
			data.b = std::stof(tokens[6]);
			data.range = std::stof(tokens[7]);
			data.intensity = std::stof(tokens[8]);
		}
		catch (...)
		{
			continue;//数値にできない行は飛ばす
		}
		m_lights.push_back(data);
	}
}

void StageLights::Update(const Vector3& playerPos)
{
	m_activeNum = 0;
	if (m_lights.empty()) return;

	//近い順に並べる(配列は毎フレーム作らないように使い回す)
	auto& nears = m_nearBuffer;
	nears.clear();
	if (m_isEnable)
	{
		for (int i = 0; i < static_cast<int>(m_lights.size()); ++i)
		{
			const float dist = (m_lights[i].pos - playerPos).Magnitude();
			if (dist < m_activeDistance) nears.emplace_back(dist, i);
		}
		std::sort(nears.begin(), nears.end());
	}

	auto& params = m_paramBuffer;
	params.clear();
	for (const auto& [dist, index] : nears)
	{
		if (static_cast<int>(params.size()) >= ModelShaderManager::kStageLightMax) break;
		const LightData& light = m_lights[index];
		//遠い端ではだんだん暗くする
		const float fade = std::clamp((m_activeDistance - dist) / m_fadeDistance, 0.0f, 1.0f);
		const float scale = light.intensity * m_colorScale * fade;
		params.push_back({ light.pos.ToDxLibVector(), light.range, GetColorF(light.r * scale, light.g * scale, light.b * scale, 1.0f) });
	}
	m_activeNum = static_cast<int>(params.size());
	ModelShaderManager::GetInstance().SetStageLights(params, m_fogStart, m_fogEnd, m_wrap);
}

void StageLights::DrawDebugWindow()
{
	if (m_lights.empty()) return;
	ImGui::Begin("Stage Lights");
	ImGui::Checkbox("Enable", &m_isEnable);
	ImGui::SliderFloat("Color Scale", &m_colorScale, 0.0f, 10.0f);
	ImGui::SliderFloat("Active Distance", &m_activeDistance, 1000.0f, 10000.0f);
	ImGui::SliderFloat("Fade Distance", &m_fadeDistance, 100.0f, 3000.0f);
	ImGui::SliderFloat("Wrap", &m_wrap, 0.0f, 1.0f);
	ImGui::Text("Lights %d / Active %d", static_cast<int>(m_lights.size()), m_activeNum);
	ImGui::End();
}

void StageLights::Release()
{
	//明かりが残らないように空を渡す
	if (!m_lights.empty()) ModelShaderManager::GetInstance().SetStageLights({}, m_fogStart, m_fogEnd, m_wrap);
	m_lights.clear();
	m_activeNum = 0;
}
