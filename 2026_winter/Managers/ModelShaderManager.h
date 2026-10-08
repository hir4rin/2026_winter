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
	//シェーダーの解放
	void Terminate();

	//trueの間、DrawModelで描くモデルを真っ黒にする(血殺中の敵用)
	void SetBlackMode(bool isBlack) { m_isBlackMode = isBlack; }
	bool GetBlackMode()const { return m_isBlackMode; }

	//MV1DrawModelの代わりに呼ぶ
	//黒くするモードのときは、モデルの頂点タイプに合った頂点シェーダー＋黒いピクセルシェーダーで描く
	void DrawModel(int modelHandle);

	//頂点タイプに合った頂点シェーダーのハンドルを返す(対応していない頂点タイプは-1)
	int GetVertexShader(int vertexType)const;

private:
	//マクロを付けてhlslをコンパイルし、DxLibのシェーダーハンドルにする
	//macrosの最後は{nullptr,nullptr}にすること
	static int LoadShaderWithMacro(const std::wstring& path, const char* target,
		const std::vector<D3D_SHADER_MACRO>& macros, bool isVertex);

	//1つのモデルを指定のピクセルシェーダーで描く
	//メッシュごとに頂点タイプが違うモデル(4ボーンと8ボーンが混ざっているなど)はメッシュごとに頂点シェーダーを切り替える
	void DrawModelWithShader(int modelHandle, int pixelShader);

private:
	std::array<int, DX_MV1_VERTEX_TYPE_NUM> m_vertexShaders = {};//添え字はDX_MV1_VERTEX_TYPE_〜
	int m_blackPixelShader = -1;//真っ黒にするピクセルシェーダー
	bool m_isInit = false;
	bool m_isBlackMode = false;
};
