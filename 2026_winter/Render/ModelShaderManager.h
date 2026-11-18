#pragma once
#include <array>
#include <string>
#include <utility>
#include <vector>
#include <d3dcompiler.h>
#include "DxLib.h"

//モデルを自作シェーダーで描画するためのシェーダーをまとめて持つ
//頂点シェーダーはモデルの頂点タイプ(1ボーン/4ボーン/8ボーン、ノーマルマップのあり/なし)ごとに必要なので、
//1つのhlslをマクロ(SKINMESH/BONE8/BUMPMAP)を変えてコンパイルし、DX_MV1_VERTEX_TYPE_〜 を添え字にして持つ
class ModelShaderManager
{
private:
	//コンストラクタとデストラクタをプライベートにして、シングルトンパターンを実装
	ModelShaderManager() = default;
	~ModelShaderManager() = default;
	ModelShaderManager(const ModelShaderManager&) = delete;
	ModelShaderManager& operator=(const ModelShaderManager&) = delete;
public:
	//シングルトンインスタンスを取得
	static ModelShaderManager& GetInstance()
	{
		static ModelShaderManager instance;
		return instance;
	}

	//シェーダーのコンパイル・読み込み
	void Init();
	//シェーダーの解放(アプリの終わりに1回)
	void Terminate();
	//シーンを出るときに、黒モード・明かりを戻す(シェーダーは次のシーンでも使うので消さない)
	void ResetSceneState();

	//trueの間、DrawModelで描くモデルを真っ黒にする(血殺中の敵用)
	void SetBlackMode(bool isBlack) { m_isBlackMode = isBlack; }
	bool GetBlackMode()const { return m_isBlackMode; }

	//黒くするモードのとき、面の裏側を描かない(トドメの真っ二つ)//切り口のそばはrimWidthの幅で白くする
	void SetCutPlane(const VECTOR& normal, float distance, float rimWidth);
	void ClearCutPlane() { m_isCutPlane = false; }

	//DrawModelで描くモデルの色を変える(ステージ用)//rateはかかり具合(0～1)
	void SetColorGrade(const COLOR_F& dark, const COLOR_F& bright, float blackLevel, float whiteLevel, float rate);
	void ClearColorGrade();

	//DrawModelで描くモデルのふちを光らせる(ガード中の敵用)//colorは色×強さ、powerはふちの細さ
	void SetGuardRim(const COLOR_F& color, float power);
	void ClearGuardRim();

	//MV1DrawModelの代わりに呼ぶ
	//黒くするモードのときは、モデルの頂点タイプに合った頂点シェーダー＋黒いピクセルシェーダーで描く
	//それ以外は DxLib のいつもの光＋ステージの明かりを1回で描く(LitModelPS.hlsl)
	void DrawModel(int modelHandle);

	//明かりが照らす場所(車内の箱の中か外か)
	enum class LightArea
	{
		Any,
		OutdoorOnly,//車内の箱の外だけ(トンネルの明かり)
		IndoorOnly,//車内の箱の中だけ(車内の明かり)
	};
	//ステージの明かり(点光源)1つ分
	struct StageLightParam
	{
		VECTOR pos;
		float range;//届く距離
		COLOR_F color;//色×強さ
		LightArea area = LightArea::Any;
	};
	static constexpr int kStageLightMax = 16;//シェーダーに渡せる明かりの数(StageLight.hlsli の LIGHT_MAX と同じ)
	static constexpr int kIndoorBoxMax = 4;//車内の箱の数(StageLight.hlsli の INDOOR_MAX と同じ)
	//今のフレームの明かりを渡す(kStageLightMax を超えた分は使わない)。遠くはフォグで消えるので fogStart/fogEnd は使っていない
	void SetStageLights(const std::vector<StageLightParam>& lights, float fogStart, float fogEnd, float wrap);
	//車内の箱(ワールド座標)。次の SetStageLights から使う。kIndoorBoxMax を超えた分は使わない
	void SetIndoorBoxes(const std::vector<std::pair<VECTOR, VECTOR>>& boxes);
	//ステージの空気感(LitModelPS.hlsl の AtmosphereBuffer)。環境光を面の向きで色分けする
	struct StageAtmosphere
	{
		COLOR_F sky;//上を向いた面に掛ける色
		COLOR_F ground;//下を向いた面に掛ける色
		COLOR_F indoorSky;//車内の上を向いた面
		COLOR_F indoorGround;//車内の下を向いた面
		VECTOR keyDir;//平行光の来る向き
		COLOR_F keyColor;
		VECTOR sideDir;//横からの光の来る向き
		COLOR_F sideColor;//(車内では使わない)
	};
	void SetAtmosphere(const StageAtmosphere& atmosphere);
	void ClearAtmosphere();

	bool IsInit()const { return m_isInit; }
	//今のフレームの明かりの定数バッファを、ピクセルシェーダーの b4 に付ける(CharacterLook が自分のシェーダーで明かりを足すとき)
	void BindStageLights();
	//法線マップ・遮蔽・光沢つきで描くピクセルシェーダー(CharacterLookPS.hlsl)。bump はノーマルマップのある頂点タイプ用
	int GetLookPixelShader(bool bump)const { return bump ? m_lookBumpPixelShader : m_lookPixelShader; }

	//頂点タイプに合った頂点シェーダーのハンドルを返す(対応していない頂点タイプは-1)
	int GetVertexShader(int vertexType)const;

	//マクロを付けてhlslをコンパイルし、DxLibのシェーダーハンドルにする(タイトルのロゴでも使う)
	//macrosの最後は{nullptr,nullptr}にすること
	static int LoadShaderWithMacro(const std::wstring& path, const char* target,
		const std::vector<D3D_SHADER_MACRO>& macros, bool isVertex);

private:
	//1つのモデルを指定のピクセルシェーダーで描く
	//メッシュごとに頂点タイプが違うモデル(4ボーンと8ボーンが混ざっているなど)はメッシュごとに頂点シェーダーを切り替える
	//bumpPixelShader はノーマルマップのある頂点タイプ用(-1 なら pixelShader)
	void DrawModelWithShader(int modelHandle, int pixelShader, int bumpPixelShader = -1);
	static bool IsBumpType(int vertexType);

private:
	//LitModelPS.hlsl の cbuffer と同じ並び
	struct StageLightConstant
	{
		FLOAT4 posRange[kStageLightMax];//xyz:位置 w:届く距離の2乗
		FLOAT4 color[kStageLightMax];//rgb:色×強さ w:1/届く距離
		FLOAT4 param;//x:数 y:車内の箱の数 z:(使っていない) w:裏側への回り込み
		FLOAT4 indoorMin[kIndoorBoxMax];
		FLOAT4 indoorMax[kIndoorBoxMax];
	};
	static constexpr int kStageLightSlot = 4;//定数バッファのスロット(StageLight.hlsli の register(b4))

	//LitModelPS.hlsl の AtmosphereBuffer と同じ並び
	struct AtmosphereConstant
	{
		FLOAT4 sky;//a:1なら使う
		FLOAT4 ground;
		FLOAT4 indoorSky;
		FLOAT4 indoorGround;
		FLOAT4 keyDir;
		FLOAT4 keyColor;
		FLOAT4 sideDir;
		FLOAT4 sideColor;
	};
	static constexpr int kAtmosphereSlot = 10;//LitModelPS.hlsl の register(b10)

	//EnemyCutPS.hlsl の cbuffer と同じ並び
	struct CutConstant
	{
		FLOAT4 plane;//xyz:法線 w:原点からの距離
		FLOAT4 param;//x:切り口の光の幅
	};
	static constexpr int kCutSlot = 6;//EnemyCutPS.hlsl の register(b6)

	//LitModelPS.hlsl の ColorGradeBuffer と同じ並び
	struct ColorGradeConstant
	{
		FLOAT4 dark;//rgb:暗い所の色 a:かかり具合
		FLOAT4 bright;//rgb:明るい所の色
		FLOAT4 level;//x:黒の明るさ y:白の明るさ
	};
	static constexpr int kColorGradeSlot = 7;//LitModelPS.hlsl の register(b7)

	//LitModelPS.hlsl の GuardRimBuffer と同じ並び
	struct GuardRimConstant
	{
		FLOAT4 color;//rgb:色×強さ
		FLOAT4 param;//x:ふちの細さ
	};
	static constexpr int kGuardRimSlot = 8;//LitModelPS.hlsl の register(b8)

	std::array<int, DX_MV1_VERTEX_TYPE_NUM> m_vertexShaders = {};//添え字はDX_MV1_VERTEX_TYPE_〜
	int m_blackPixelShader = -1;//真っ黒にするピクセルシェーダー
	int m_litPixelShader = -1;//いつもの光＋明かりで描くピクセルシェーダー
	int m_litBumpPixelShader = -1;//同じ(ノーマルマップのある頂点タイプ用)
	int m_cutPixelShader = -1;//真っ二つにするピクセルシェーダー
	int m_cutBumpPixelShader = -1;//同じ(ノーマルマップのある頂点タイプ用)
	int m_cutConstantBuffer = -1;
	bool m_isCutPlane = false;
	int m_colorGradeBuffer = -1;
	bool m_isColorGrade = false;
	int m_guardRimBuffer = -1;
	bool m_isGuardRim = false;
	int m_lightConstantBuffer = -1;
	int m_atmosphereBuffer = -1;
	std::vector<std::pair<VECTOR, VECTOR>> m_indoorBoxes;
	int m_lookPixelShader = -1;
	int m_lookBumpPixelShader = -1;
	int m_stageLightNum = 0;
	bool m_isInit = false;
	bool m_isBlackMode = false;
};
