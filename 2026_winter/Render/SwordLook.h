#pragma once
#include "DxLib.h"
#include <vector>

/// <summary>
/// プレイヤーの刀を専用のシェーダーで描く
/// 通常の刀(katana_blend): 鏡のような鋼・刃文・刃先の光(Render/Shader/SwordPS.hlsl)
/// 必殺技の長い刀(red_katana_ult): 黒い鋼に流れる血の光(Render/Shader/UltSwordPS.hlsl)
/// 平行光・環境光は CharacterLook::Settings() と同じものを使う(体と刀の光をそろえる)
/// </summary>
class SwordLook
{
public:
	//通常の刀の見た目の設定
	struct LookSettings
	{
		bool isEnable = true;//false なら今まで通りの描き方(見比べ用)
		COLOR_F envSky = GetColorF(0.42f, 0.48f, 0.6f, 1.0f);//映り込みの空
		COLOR_F envGround = GetColorF(0.03f, 0.03f, 0.04f, 1.0f);//映り込みの地面
		COLOR_F envHorizon = GetColorF(1.1f, 1.05f, 1.0f, 1.0f);//映り込みの地平線の光の帯
		COLOR_F envStrip = GetColorF(1.4f, 1.5f, 1.7f, 1.0f);//映り込みの縦の光の帯
		float envStrength = 1.0f;//映り込みの強さ
		COLOR_F steel = GetColorF(0.55f, 0.58f, 0.64f, 1.0f);//地鉄の色
		float steelRough = 0.08f;//地鉄のざらつき
		COLOR_F hamon = GetColorF(0.85f, 0.88f, 0.95f, 1.0f);//刃文の色
		float hamonLine = 0.28f;//刃文の位置(刃先0～峰1)
		COLOR_F edge = GetColorF(0.45f, 0.75f, 1.0f, 1.0f);//刃先の光の色
		float edgeIntensity = 0.35f;//ふだんの刃先の光
		float edgeWidth = 0.07f;//刃先の光の幅(刃先0～峰1)
		float edgeSlash = 2.5f;//斬っている間に足す刃先の光
		COLOR_F sweep = GetColorF(0.9f, 0.95f, 1.0f, 1.0f);//根元から先へ走る光の色
		float sweepWidth = 0.07f;//走る光の幅(刀の長さ0～1)
		float sweepPeriod = 150.0f;//走る光の間隔(フレーム)
		float rimStrength = 0.5f;//ふちの光
		float slashEnvBoost = 0.8f;//斬っている間の映り込みの強さの足し分
		float slashRise = 0.35f;//斬りの光が出る速さ(1フレーム)
		float slashFall = 0.06f;//斬りの光が消える速さ(1フレーム)
		//刀身の UV(katana_blend の T_SM_Katana_A の左の帯。真ん中が刃先、両端が峰)
		float bladeCenterU = 0.0835f;
		float bladeHalfU = 0.0725f;
		float bladeBaseV = 0.995f;
		float bladeTipV = 0.004f;
	};
	static LookSettings& Settings();

	//必殺技の長い刀の見た目の設定
	struct UltLookSettings
	{
		bool isEnable = true;
		COLOR_F envSky = GetColorF(0.32f, 0.07f, 0.09f, 1.0f);//映り込みの空(赤黒い)
		COLOR_F envHorizon = GetColorF(1.0f, 0.3f, 0.25f, 1.0f);//映り込みの地平線の帯
		COLOR_F envStrip = GetColorF(1.1f, 0.55f, 0.5f, 1.0f);//映り込みの縦の帯
		float envStrength = 1.0f;//映り込みの強さ
		COLOR_F core = GetColorF(0.22f, 0.2f, 0.21f, 1.0f);//黒い鋼の反射の強さ
		float coreRough = 0.07f;//黒い鋼のざらつき
		COLOR_F bloodDeep = GetColorF(0.45f, 0.0f, 0.04f, 1.0f);//血の暗い色
		COLOR_F bloodBright = GetColorF(1.0f, 0.16f, 0.12f, 1.0f);//血の明るい色
		COLOR_F edgeHot = GetColorF(1.0f, 0.72f, 0.6f, 1.0f);//刃先の熱い色
		COLOR_F vein = GetColorF(1.0f, 0.1f, 0.12f, 1.0f);//血管の色
		float veinIntensity = 0.9f;//血管の強さ
		float flowSpeed = 0.012f;//血の流れる速さ(1フレーム)
		float pulsePeriod = 55.0f;//脈の間隔(フレーム)
		float rimStrength = 0.6f;//ふちの赤い光
		float slashBoost = 0.6f;//斬っている間・伸びている間の明るさの足し分
		//刀身の形(red_katana_ult のフレームの空間。z が長さ、y が幅で + が刃先)
		float tipZ = -5.26f;
		float baseZ = -0.29f;
		float borderY = 0.017f;//黒(峰側)と赤(刃側)の境目
		float edgeY = 0.058f;//刃先
		float spineY = -0.0208f;//峰
	};
	static UltLookSettings& UltSettings();

	SwordLook() = default;
	~SwordLook();

	/// <summary>通常の刀として使う。モデルのマテリアルの法線マップ・マスク画像を使う。読めなければ Draw は今まで通りの描き方</summary>
	bool Load(int modelHandle);
	/// <summary>必殺技の長い刀として使う。isBladeMesh は刀身のメッシュか(添え字はメッシュ番号)</summary>
	bool LoadUlt(int modelHandle, const std::vector<bool>& isBladeMesh);
	/// <summary>斬りの光・時間を進める。isSlashing は斬っている間 true(1フレームに1回呼ぶ)</summary>
	void Update(bool isSlashing);
	/// <summary>MV1DrawModel の代わりに呼ぶ(MV1SetMeshVisible で隠したメッシュは描かない)</summary>
	void Draw(int modelHandle);
	/// <summary>設定の ImGui ウィンドウ(F2 の表示中)</summary>
	static void DrawDebugWindow();

private:
	//メッシュの部品(UltSwordPS.hlsl の部品の番号と同じ)
	enum class Part
	{
		Fitting = 0,//柄・鍔
		Core = 1,//峰側(黒)
		Edge = 2,//刃側(赤く光る)
	};

	//SwordPS.hlsl の SwordBuffer と同じ並び
	struct SwordConstant
	{
		FLOAT4 cameraPos;
		FLOAT4 keyDir;
		FLOAT4 keyColor;
		FLOAT4 skyColor;
		FLOAT4 groundColor;
		FLOAT4 envSky;
		FLOAT4 envGround;
		FLOAT4 envHorizon;
		FLOAT4 envStrip;
		FLOAT4 steel;
		FLOAT4 hamon;
		FLOAT4 edge;
		FLOAT4 sweep;
		FLOAT4 param;
		FLOAT4 blade;
	};
	static constexpr int kSwordSlot = 11;//SwordPS.hlsl の register(b11)

	//UltSwordPS.hlsl の UltSwordBuffer と同じ並び
	struct UltConstant
	{
		FLOAT4 cameraPos;
		FLOAT4 keyDir;
		FLOAT4 keyColor;
		FLOAT4 skyColor;
		FLOAT4 groundColor;
		FLOAT4 envSky;
		FLOAT4 envHorizon;
		FLOAT4 envStrip;
		FLOAT4 core;
		FLOAT4 bloodDeep;
		FLOAT4 bloodBright;
		FLOAT4 edgeHot;
		FLOAT4 vein;
		FLOAT4 shape;
		FLOAT4 shape2;
		FLOAT4 toLocal[4];
	};
	static constexpr int kUltSlot = 12;//UltSwordPS.hlsl の register(b12)

	void Release();
	void SetConstant();
	void SetUltConstant(int modelHandle, Part part);
	bool IsEnable() const;
	int GetPixelShader(bool isBump) const;
	//マテリアルの画像のハンドル(無ければ -1)
	static int GetMaterialGraph(int modelHandle, int textureIndex);

	bool m_isUlt = false;//必殺技の長い刀か
	std::vector<Part> m_meshParts;//メッシュごとの部品(長い刀だけ)
	int m_normalTex = -1;//モデルの持ち物なので消さない
	int m_maskTex = -1;
	int m_constantBuffer = -1;
	bool m_isLoaded = false;
	float m_frame = 0.0f;//走る光・脈の時間
	float m_slashRate = 0.0f;//斬りの光(0～1)
};
