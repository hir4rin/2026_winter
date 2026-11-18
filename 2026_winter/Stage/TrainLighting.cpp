#include "TrainLighting.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>

namespace
{
	//トンネル(TrainTunnel.mv1)の中の明かりの位置。区間(3300)ごとに、真ん中にリング照明、前後833に天井灯
	constexpr float kSectionLength = 3300.0f;
	constexpr int kSectionNum = 4;//トンネル1本の区間の数
	constexpr float kLampOffsetZ = 833.0f;//区間の真ん中から天井灯まで
	constexpr float kLampY = 1250.0f;//天井灯の高さ(トンネルの中)
	constexpr float kRingY = 650.0f;//リング照明の明かりの高さ(輪の真ん中あたり)

	//トンネルの明かりの色・届く距離
	const COLOR_F kLampColor = GetColorF(0.55f, 0.8f, 1.0f, 1.0f);//水色がかった白
	constexpr float kLampStrength = 0.45f;
	constexpr float kLampRange = 1600.0f;
	const COLOR_F kRingColor = GetColorF(0.3f, 0.78f, 1.0f, 1.0f);//水色
	constexpr float kRingStrength = 2.4f;
	constexpr float kRingRange = 1300.0f;
	//トンネルの発光(見た目)の色
	const COLOR_F kLampEmissive = GetColorF(0.8f, 0.95f, 1.0f, 1.0f);
	const COLOR_F kRingEmissive = GetColorF(0.3f, 0.85f, 1.0f, 1.0f);

	//通路の車両(TrainStage.mv1 の corridor_shell。z の範囲)
	constexpr float kCarRanges[][2] = { { 4250.0f, 8296.0f }, { 10697.0f, 14744.0f }, { 17144.0f, 21190.0f } };
	constexpr float kCarHalfWidth = 780.0f;//車内の箱の半分の幅(外の面は ±795)
	constexpr float kCarFloorY = -40.0f;
	constexpr float kCarCeilingY = 590.0f;//屋根の上(602)は外
	constexpr float kCarEndMargin = 12.0f;//前後の壁の外の面は外
	//車内の明かり
	const COLOR_F kCarColor = GetColorF(1.0f, 0.5f, 0.2f, 1.0f);//橙
	constexpr float kCarStrength = 1.6f;
	constexpr float kCarRange = 1100.0f;
	constexpr float kCarLightY = 500.0f;
	constexpr float kCarLightStart = 500.0f;//車両の始まりから最初の明かりまで
	constexpr float kCarLightInterval = 1000.0f;

	//明かりを選ぶ距離
	constexpr float kActiveDistance = 2500.0f;//プレイヤーからこの距離までの明かりを使う(遠くはフォグで暗い)
	constexpr float kFadeDistance = 800.0f;//遠い端のこの幅でだんだん暗くする
	constexpr float kWrap = 0.15f;//光に背を向けた面もこの割合だけ照らす

	//キャラが車内にいるかを見る高さ(足元から)と、光を切り替える速さ
	constexpr float kPlayerCheckHeight = 100.0f;
	constexpr float kIndoorBlendSpeed = 0.08f;

	//空気感(いつもの光の環境光に掛ける)
	const ModelShaderManager::StageAtmosphere kAtmosphere =
	{
		GetColorF(0.18f, 0.22f, 0.36f, 1.0f),//上を向いた面(紺)
		GetColorF(0.07f, 0.07f, 0.11f, 1.0f),//下を向いた面
		GetColorF(0.80f, 0.52f, 0.32f, 1.0f),//車内の上(橙)
		GetColorF(0.36f, 0.22f, 0.14f, 1.0f),//車内の下
		VGet(0.2f, 1.0f, -0.3f),//平行光の向き(ほぼ上)
		GetColorF(0.10f, 0.14f, 0.24f, 1.0f),//平行光(青)
		VGet(1.0f, 0.15f, 0.0f),//横からの光の向き
		GetColorF(0.45f, 0.2f, 0.06f, 1.0f),//横からの光(橙)
	};

	//プレイヤー(CharacterLook)の光。外と車内
	const VECTOR kLookKeyDir = VGet(0.3f, 1.0f, -0.4f);
	const COLOR_F kLookKey = GetColorF(0.3f, 0.4f, 0.62f, 1.0f);
	const COLOR_F kLookSky = GetColorF(0.14f, 0.18f, 0.3f, 1.0f);
	const COLOR_F kLookGround = GetColorF(0.05f, 0.05f, 0.07f, 1.0f);
	const COLOR_F kLookRim = GetColorF(0.6f, 0.32f, 0.12f, 1.0f);
	const COLOR_F kLookIndoorKey = GetColorF(0.55f, 0.38f, 0.24f, 1.0f);
	const COLOR_F kLookIndoorSky = GetColorF(0.4f, 0.27f, 0.17f, 1.0f);
	const COLOR_F kLookIndoorGround = GetColorF(0.15f, 0.09f, 0.06f, 1.0f);
	const COLOR_F kLookIndoorRim = GetColorF(0.45f, 0.22f, 0.08f, 1.0f);
}

TrainLighting::~TrainLighting()
{
	if (!m_isInit) return;
	//ほかのステージに残らないように戻す
	CharacterLook::Settings() = m_savedLook;
	auto& shaderManager = ModelShaderManager::GetInstance();
	shaderManager.ClearAtmosphere();
	shaderManager.SetIndoorBoxes({});
	shaderManager.SetStageLights({}, 0.0f, 0.0f, 0.0f);
}

void TrainLighting::Init(int tunnelHandle, const Vector3& stagePos, float tunnelOffsetY)
{
	m_tunnelBase = Vector3(stagePos.x, stagePos.y + tunnelOffsetY, stagePos.z);

	//トンネルの発光の色を変える(白っぽいのが天井灯、黄色っぽいのがリング照明)
	for (int i = 0; i < MV1GetMaterialNum(tunnelHandle); ++i)
	{
		const COLOR_F emi = MV1GetMaterialEmiColor(tunnelHandle, i);
		if (emi.r + emi.g + emi.b < 0.5f) continue;
		MV1SetMaterialEmiColor(tunnelHandle, i, emi.b < 0.9f ? kRingEmissive : kLampEmissive);
	}

	//トンネル1本の中の明かり
	m_tunnelLights.clear();
	for (int s = 0; s < kSectionNum; ++s)
	{
		const float center = (s - (kSectionNum - 1) * 0.5f) * kSectionLength;
		m_tunnelLights.push_back({ Kind::Ring, Vector3(0.0f, kRingY, center), kRingColor, kRingRange, ModelShaderManager::LightArea::OutdoorOnly });
		for (float side : { -1.0f, 1.0f })
		{
			m_tunnelLights.push_back({ Kind::Lamp, Vector3(0.0f, kLampY, center + side * kLampOffsetZ), kLampColor, kLampRange, ModelShaderManager::LightArea::OutdoorOnly });
		}
	}

	//車内の箱と明かり
	m_indoorBoxes.clear();
	m_carLights.clear();
	for (const auto& range : kCarRanges)
	{
		m_indoorBoxes.push_back({
			VAdd(stagePos.ToDxLibVector(), VGet(-kCarHalfWidth, kCarFloorY, range[0] + kCarEndMargin)),
			VAdd(stagePos.ToDxLibVector(), VGet(kCarHalfWidth, kCarCeilingY, range[1] - kCarEndMargin)) });
		for (float z = range[0] + kCarLightStart; z < range[1]; z += kCarLightInterval)
		{
			m_carLights.push_back({ Kind::Car, stagePos + Vector3(0.0f, kCarLightY, z), kCarColor, kCarRange, ModelShaderManager::LightArea::IndoorOnly });
		}
	}
	ModelShaderManager::GetInstance().SetIndoorBoxes(m_indoorBoxes);

	m_savedLook = CharacterLook::Settings();
	m_atmosphere = kAtmosphere;
	ApplyAtmosphere();
	m_isInit = true;
	ApplyCharacterLook();
}

void TrainLighting::Update(const Vector3& playerPos, float tunnelScroll, float tunnelLength)
{
	if (!m_isInit) return;

	//プレイヤーが車内にいるか(キャラの光をなめらかに切り替える)
	const float target = IsIndoor(playerPos + Vector3(0.0f, kPlayerCheckHeight, 0.0f)) ? 1.0f : 0.0f;
	m_indoorRate += std::clamp(target - m_indoorRate, -kIndoorBlendSpeed, kIndoorBlendSpeed);
	ApplyCharacterLook();

	m_candidates.clear();
	if (m_isEnable)
	{
		//トンネルの明かり(Stage::DrawTunnel と同じ位置の計算。トンネルと一緒に流れる)
		const float baseZ = m_tunnelBase.z + tunnelScroll;
		const float halfLength = tunnelLength * 0.5f;
		const int first = static_cast<int>(std::ceil((playerPos.z - kActiveDistance - halfLength - baseZ) / tunnelLength));
		const int last = static_cast<int>(std::floor((playerPos.z + kActiveDistance + halfLength - baseZ) / tunnelLength));
		for (int i = first; i <= last; ++i)
		{
			const Vector3 tunnelPos(m_tunnelBase.x, m_tunnelBase.y, baseZ + i * tunnelLength);
			for (const Light& local : m_tunnelLights)
			{
				Light light = local;
				light.pos = tunnelPos + local.pos;
				AddCandidate(light, playerPos);
			}
		}
		for (const Light& light : m_carLights)
		{
			AddCandidate(light, playerPos);
		}
		std::sort(m_candidates.begin(), m_candidates.end());
	}

	auto& params = m_paramBuffer;
	params.clear();
	for (const Candidate& c : m_candidates)
	{
		if (static_cast<int>(params.size()) >= ModelShaderManager::kStageLightMax) break;
		const float fade = std::clamp((kActiveDistance - c.dist) / kFadeDistance, 0.0f, 1.0f);
		const COLOR_F& col = c.light.color;
		params.push_back({ c.light.pos.ToDxLibVector(), c.light.range, GetColorF(col.r * fade, col.g * fade, col.b * fade, 1.0f), c.light.area });
	}
	m_activeNum = static_cast<int>(params.size());
	ModelShaderManager::GetInstance().SetStageLights(params, 0.0f, 0.0f, kWrap);
}

void TrainLighting::AddCandidate(const Light& light, const Vector3& playerPos)
{
	const float dist = (light.pos - playerPos).Magnitude();
	if (dist >= kActiveDistance) return;
	//強さはここで掛ける(ImGui で変えられるように)
	float strength = kCarStrength * m_carIntensity;
	if (light.kind == Kind::Lamp) strength = kLampStrength * m_lampIntensity;
	if (light.kind == Kind::Ring) strength = kRingStrength * m_ringIntensity;
	Candidate c = { dist, light };
	c.light.color = GetColorF(light.color.r * strength, light.color.g * strength, light.color.b * strength, 1.0f);
	m_candidates.push_back(c);
}

bool TrainLighting::IsIndoor(const Vector3& pos)const
{
	for (const auto& [boxMin, boxMax] : m_indoorBoxes)
	{
		if (pos.x >= boxMin.x && pos.y >= boxMin.y && pos.z >= boxMin.z &&
			pos.x <= boxMax.x && pos.y <= boxMax.y && pos.z <= boxMax.z) return true;
	}
	return false;
}

void TrainLighting::ApplyAtmosphere()
{
	ModelShaderManager::GetInstance().SetAtmosphere(m_atmosphere);
}

void TrainLighting::ApplyCharacterLook()
{
	auto lerpColor = [](const COLOR_F& a, const COLOR_F& b, float t)
		{
			return GetColorF(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, 1.0f);
		};
	auto& look = CharacterLook::Settings();
	look.keyDir = kLookKeyDir;
	look.keyColor = lerpColor(kLookKey, kLookIndoorKey, m_indoorRate);
	look.skyColor = lerpColor(kLookSky, kLookIndoorSky, m_indoorRate);
	look.groundColor = lerpColor(kLookGround, kLookIndoorGround, m_indoorRate);
	look.rimColor = lerpColor(kLookRim, kLookIndoorRim, m_indoorRate);
}

void TrainLighting::DrawDebugWindow()
{
	if (!m_isInit) return;
	ImGui::Begin("Train Lighting");
	ImGui::Checkbox("Lights", &m_isEnable);
	ImGui::SliderFloat("Lamp", &m_lampIntensity, 0.0f, 4.0f);
	ImGui::SliderFloat("Ring", &m_ringIntensity, 0.0f, 4.0f);
	ImGui::SliderFloat("Car", &m_carIntensity, 0.0f, 4.0f);
	bool changed = false;
	changed |= ImGui::ColorEdit3("Sky", &m_atmosphere.sky.r, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Ground", &m_atmosphere.ground.r, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Key", &m_atmosphere.keyColor.r, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Side", &m_atmosphere.sideColor.r, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Indoor Sky", &m_atmosphere.indoorSky.r, ImGuiColorEditFlags_Float);
	changed |= ImGui::ColorEdit3("Indoor Ground", &m_atmosphere.indoorGround.r, ImGuiColorEditFlags_Float);
	if (changed) ApplyAtmosphere();
	ImGui::Text("Active %d / Indoor %.2f", m_activeNum, m_indoorRate);
	ImGui::End();
}
