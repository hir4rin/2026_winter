#pragma once
#include "DxLib.h"
#include "Math/Vector3.h"
#include <memory>
#include <vector>

class SkyBox;

/// <summary>
/// 列車ステージのエンディングで、トンネルを抜けた後の景色
/// ステージ1の夜の空(遠くの街)・空の窓に重ねる光の粒・線路脇を流れる灯り
/// </summary>
class TrainOutside
{
public:
	TrainOutside();
	~TrainOutside();

	/// <param name="stagePos">ステージのモデルの位置</param>
	/// <param name="baseY">灯りの柱の根元の高さ(ステージの位置から)</param>
	/// <param name="forwardZ">列車の進む向き(+1か-1)</param>
	void Init(const Vector3& stagePos, float baseY, float forwardZ);

	/// <summary>外の見え方(0:見えない〜1:霧が晴れた)</summary>
	void SetRate(float rate) { m_rate = rate; }
	float GetRate()const { return m_rate; }

	/// <summary>空(ステージより先に、Zを使わずに描く)</summary>
	void DrawSky()const;
	/// <summary>線路脇の灯りの柱(フォグをかけてステージと一緒に描く)</summary>
	void DrawLamps(float exitZ)const;
	/// <summary>灯りの頭と遠くの街の光の粒(ステージの後に、フォグを切って描く)</summary>
	void DrawGlows(float exitZ)const;
	/// <summary>灯りの頭の位置(流れる明かりに使う)</summary>
	void GetLampLights(float exitZ, float centerZ, float range, std::vector<Vector3>& out)const;

private:
	struct Lamp
	{
		Vector3 base;//柱の根元
		Vector3 head;//灯りの頭
		float rotY;
	};
	struct CityGlow
	{
		VECTOR dir;//カメラから見た向き(長さ1)
		int r, g, b;
		float size;
		float alpha;
		float phase;
	};

	/// <summary>exitZ より先(外)で、centerZ から range の中の灯り</summary>
	void CollectLamps(float exitZ, float centerZ, float range, std::vector<Lamp>& out)const;
	/// <summary>空の絵の窓(周りより明るい点)を拾って光の粒にする</summary>
	void LoadCityGlows();
	void MakeGlowGraph();

	std::unique_ptr<SkyBox> m_skyBox;
	int m_lampHandle = -1;
	int m_glowGraph = -1;
	Vector3 m_stagePos;
	float m_baseY = 0.0f;
	float m_forwardZ = 1.0f;
	float m_rate = 0.0f;
	std::vector<CityGlow> m_cityGlows;
	mutable std::vector<Lamp> m_lampBuffer;//描くときに使い回す
};
