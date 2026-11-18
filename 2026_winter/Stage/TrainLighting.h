#pragma once
#include "Math/Vector3.h"
#include "Render/CharacterLook.h"
#include "Render/ModelShaderManager.h"
#include <utility>
#include <vector>

/// <summary>
/// 列車ステージのライティング(NINJA GAIDEN 4 の列車を参考に)
/// 全体は暗い紺、トンネルの天井灯・リング照明の明かりをトンネルと一緒に流す、通路の車両の中は暖色(外の明かりは入れない)
/// </summary>
class TrainLighting
{
public:
	TrainLighting() = default;
	~TrainLighting();

	/// <summary>トンネルの発光の色を変え、空気感・キャラの光を列車用にする</summary>
	/// <param name="stagePos">ステージのモデルの位置</param>
	/// <param name="tunnelOffsetY">トンネルを置く高さ(ステージの位置から)</param>
	void Init(int tunnelHandle, const Vector3& stagePos, float tunnelOffsetY);
	/// <summary>プレイヤーに近い明かりをシェーダーに渡す(毎フレーム)</summary>
	void Update(const Vector3& playerPos, float tunnelScroll, float tunnelLength);
	/// <summary>調整用の ImGui ウィンドウ</summary>
	void DrawDebugWindow();

private:
	enum class Kind { Lamp, Ring, Car };
	struct Light
	{
		Kind kind;
		Vector3 pos;
		COLOR_F color;
		float range = 0.0f;
		ModelShaderManager::LightArea area = ModelShaderManager::LightArea::Any;
	};
	struct Candidate
	{
		float dist;
		Light light;
		bool operator<(const Candidate& other)const { return dist < other.dist; }
	};

	void ApplyAtmosphere();
	void ApplyCharacterLook();
	bool IsIndoor(const Vector3& pos)const;
	void AddCandidate(const Light& light, const Vector3& playerPos);

	bool m_isInit = false;
	Vector3 m_tunnelBase;//トンネルを置く位置(z は流す前)
	std::vector<Light> m_tunnelLights;//トンネル1本の中の明かり(トンネルからの位置)
	std::vector<Light> m_carLights;//車内の明かり(ワールド座標)
	std::vector<std::pair<VECTOR, VECTOR>> m_indoorBoxes;//車内の箱(ワールド座標)
	std::vector<Candidate> m_candidates;//Update で使い回す
	std::vector<ModelShaderManager::StageLightParam> m_paramBuffer;
	CharacterLook::LookSettings m_savedLook;//戻す用
	float m_indoorRate = 0.0f;//プレイヤーが車内にいる割合(キャラの光を切り替える)
	int m_activeNum = 0;

	//調整できる値(ImGui)
	bool m_isEnable = true;
	float m_lampIntensity = 1.0f;
	float m_ringIntensity = 1.0f;
	float m_carIntensity = 1.0f;
	ModelShaderManager::StageAtmosphere m_atmosphere = {};
};
