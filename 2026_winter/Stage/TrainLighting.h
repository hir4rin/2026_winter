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

	//エンディング(トンネルを抜ける)
	/// <summary>出口より先のトンネルの明かりは使わない</summary>
	void SetTunnelExit(float exitZ, float forwardZ) { m_hasExit = true; m_exitZ = exitZ; m_forwardZ = forwardZ; }
	/// <summary>線路脇の灯りの位置(次の Update から使う)</summary>
	void SetTrackLamps(const std::vector<Vector3>& heads) { m_trackLamps = heads; }
	/// <summary>外の空気感にする割合(0:トンネル〜1:外の夜)</summary>
	void SetOutsideRate(float rate);

private:
	enum class Kind { Lamp, Ring, Car, Track };
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
	static COLOR_F LerpColor(const COLOR_F& a, const COLOR_F& b, float t);
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
	bool m_hasExit = false;//トンネルの出口があるか(エンディング)
	float m_exitZ = 0.0f;
	float m_forwardZ = 1.0f;
	std::vector<Vector3> m_trackLamps;//線路脇の灯りの頭(ワールド座標)
	float m_outsideRate = 0.0f;

	//調整できる値(ImGui)
	bool m_isEnable = true;
	float m_lampIntensity = 1.0f;
	float m_ringIntensity = 1.0f;
	float m_carIntensity = 1.0f;
	float m_trackIntensity = 1.0f;
	ModelShaderManager::StageAtmosphere m_atmosphere = {};
};
