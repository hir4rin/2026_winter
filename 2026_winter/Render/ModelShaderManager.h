#pragma once
#include <array>
#include <string>
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

	//MV1DrawModelの代わりに呼ぶ
	//黒くするモードのときは、モデルの頂点タイプに合った頂点シェーダー＋黒いピクセルシェーダーで描く
	//それ以外は DxLib のいつもの光＋ステージの明かりを1回で描く(LitModelPS.hlsl)
	void DrawModel(int modelHandle);

	//ステージの明かり(点光源)1つ分
	struct StageLightParam
	{
		VECTOR pos;
		float range;//届く距離
		COLOR_F color;//色×強さ
	};
	static constexpr int kStageLightMax = 16;//シェーダーに渡せる明かりの数(LitModelPS.hlsl の LIGHT_MAX と同じ)
	//今のフレームの明かりを渡す(kStageLightMax を超えた分は使わない)。遠くはフォグで消えるので fogStart/fogEnd は使っていない
	void SetStageLights(const std::vector<StageLightParam>& lights, float fogStart, float fogEnd, float wrap);

	bool IsInit()const { return m_isInit; }
	//今のフレームの明かりの定数バッファを、ピクセルシェーダーの b4 に付ける(CharacterLook が自分のシェーダーで明かりを足すとき)
	void BindStageLights();
	//法線マップ・遮蔽・光沢つきで描くピクセルシェーダー(CharacterLookPS.hlsl)。bump はノーマルマップのある頂点タイプ用
	int GetLookPixelShader(bool bump)const { return bump ? m_lookBumpPixelShader : m_lookPixelShader; }

	//頂点タイプに合った頂点シェーダーのハンドルを返す(対応していない頂点タイプは-1)
	int GetVertexShader(int vertexType)const;

private:
	//マクロを付けてhlslをコンパイルし、DxLibのシェーダーハンドルにする
	//macrosの最後は{nullptr,nullptr}にすること
	static int LoadShaderWithMacro(const std::wstring& path, const char* target,
		const std::vector<D3D_SHADER_MACRO>& macros, bool isVertex);

	//1つのモデルを指定のピクセルシェーダーで描く
	//メッシュごとに頂点タイプが違うモデル(4ボーンと8ボーンが混ざっているなど)はメッシュごとに頂点シェーダーを切り替える
	//bumpPixelShader はノーマルマップのある頂点タイプ用(-1 なら pixelShader)
	void DrawModelWithShader(int modelHandle, int pixelShader, int bumpPixelShader = -1);
	static bool IsBumpType(int vertexType);

private:
	//LitModelPS.hlsl の cbuffer と同じ並び
	struct StageLightConstant
	{
		FLOAT4 posRange[kStageLightMax];//xyz:位置 w:届く距離
		FLOAT4 color[kStageLightMax];
		FLOAT4 param;//x:数 y:フォグの始まり z:フォグの終わり w:裏側への回り込み
	};
	static constexpr int kStageLightSlot = 4;//定数バッファのスロット(LitModelPS.hlsl の register(b4))

	std::array<int, DX_MV1_VERTEX_TYPE_NUM> m_vertexShaders = {};//添え字はDX_MV1_VERTEX_TYPE_〜
	int m_blackPixelShader = -1;//真っ黒にするピクセルシェーダー
	int m_litPixelShader = -1;//いつもの光＋明かりで描くピクセルシェーダー
	int m_litBumpPixelShader = -1;//同じ(ノーマルマップのある頂点タイプ用)
	int m_lightConstantBuffer = -1;
	int m_lookPixelShader = -1;
	int m_lookBumpPixelShader = -1;
	int m_stageLightNum = 0;
	bool m_isInit = false;
	bool m_isBlackMode = false;
};
