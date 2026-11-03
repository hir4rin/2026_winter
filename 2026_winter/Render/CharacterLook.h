#pragma once
#include "DxLib.h"
#include <map>
#include <string>
#include <vector>

/// <summary>
/// キャラを「法線マップ・遮蔽・光沢つき」で描く(Render/Shader/CharacterLookPS.hlsl)
/// マテリアルの名前ごとの画像と強さは CSV(例 data/Player/Look/PlayerLook.csv)から読む。画像は CSV と同じフォルダ
/// 光(平行光・環境光・ふちの光)は全キャラ共通の Settings()。ステージの明かりも同じシェーダーで足す
/// </summary>
class CharacterLook
{
public:
	//全キャラ共通の光の設定
	struct LookSettings
	{
		bool isEnable = true;//false なら今まで通りの描き方(見比べ用)
		VECTOR keyDir = VGet(1.0f, 1.0f, -1.0f);//光の来る向き(SceneMain の平行光 -1,-1,1 の逆)
		COLOR_F keyColor = GetColorF(0.85f, 0.88f, 0.95f, 1.0f);//平行光(月明かり)
		COLOR_F skyColor = GetColorF(0.5f, 0.52f, 0.6f, 1.0f);//上からの環境光
		COLOR_F groundColor = GetColorF(0.28f, 0.26f, 0.25f, 1.0f);//下からの環境光
		COLOR_F rimColor = GetColorF(0.25f, 0.32f, 0.5f, 1.0f);//ふちの光
		float rimPower = 3.0f;//ふちの光の細さ
		float normalFlipY = 1.0f;//法線マップの緑の向き(1 か -1)
	};
	static LookSettings& Settings();

	CharacterLook() = default;
	~CharacterLook();

	/// <summary>CSV を読み、モデルのマテリアルごとに画像と強さを用意する。読めなければ Draw は今まで通りの描き方になる</summary>
	bool Load(int modelHandle, const std::string& csvPath);
	/// <summary>MV1DrawModel の代わりに呼ぶ</summary>
	void Draw(int modelHandle);
	/// <summary>光の設定の ImGui ウィンドウ(F2 の表示中)</summary>
	static void DrawDebugWindow();
	/// <summary>CSV の画像だけ先に読んでおく(起動時の非同期読み込みの間に呼ぶ。Load ではこれを使い回す)</summary>
	static void PreloadGraphs(const std::string& csvPath);
	/// <summary>使い回している画像を解放する(アプリの終わりに1回)</summary>
	static void ReleaseGraphCache();

private:
	//1マテリアル分
	struct MaterialLook
	{
		int normalTex = -1;
		int maskTex = -1;
		float normalStrength = 1.0f;
		float aoStrength = 1.0f;
		float smoothStrength = 1.0f;
		float metalStrength = 0.0f;
		float specStrength = 1.0f;
		float alphaClip = 0.01f;//これより薄い所は描かない
		float tint[3] = { 1.0f, 1.0f, 1.0f };//色に掛ける
		bool drawLast = false;//半透明(髪・まつげ)は後で描く
	};
	//CharacterLookPS.hlsl の LookBuffer と同じ並び
	struct LookConstant
	{
		FLOAT4 keyDir;
		FLOAT4 keyColor;
		FLOAT4 skyColor;
		FLOAT4 groundColor;
		FLOAT4 cameraPos;
		FLOAT4 rim;
		FLOAT4 mat0;
		FLOAT4 mat1;
		FLOAT4 tint;
	};
	static constexpr int kLookSlot = 5;//定数バッファのスロット(CharacterLookPS.hlsl の register(b5))

	void Release();
	void SetMaterialConstant(const MaterialLook& look);
	//読んだ画像(パス→ハンドル)。読み込みが重いのでシーンをまたいで使い回す
	static std::map<std::string, int>& GraphCache();
	static int LoadCachedGraph(const std::string& dir, const std::string& name);

	std::vector<MaterialLook> m_materials;//添え字はモデルのマテリアル番号
	int m_constantBuffer = -1;
	bool m_isLoaded = false;
};
