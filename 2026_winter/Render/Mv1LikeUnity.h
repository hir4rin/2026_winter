// Mv1LikeUnity: DxLib の MV1 モデルを Unity(HDRP)と同じ見た目で描くための部品(ヘッダだけ。DxLib.h のあとに include する)。
// IdaFaber(SK_VEX_*)の移植で作った実装で、今のところ表面の計算(Surface.hlsli)・段階別出力(Stage3_PS.hlsl)は IdaFaber 固有。
// 光の計算(Lighting.hlsli)・事前積分表(FGD_PS.hlsl)・8bit への変換(Resolve_PS.hlsl)は Unity 本体の計算なので使い回せる。
//
// 使い方(5 段階。① モデルを読む(DxLib 標準)以外はこのヘッダの関数を呼ぶ。説明書は Mv1LikeUnity_リファレンス.md、
// 中身を 1 つずつ紐解く教材は lessons\Mv1LikeUnity_教材.md):
//
//     #include "DxLib.h"
//     #include "Mv1LikeUnity.h"
//     ...
//     if (DxLib_Init() == -1) return -1;
//     Mv1LikeUnity::InitLoadShaders();                                        // ② シェーダーの準備。DxLib_Init の直後に 1 回(カメラを設定する前に)
//     const int model = MV1LoadModel(_T("SK_VEX_01/SK_VEX_01.mv1"));      // ① モデルを読む(DxLib 標準の MV1LoadModel をそのまま使う)
//     auto& mats = Mv1LikeUnity::LoadMaterials(model, _T("SK_VEX_01/SK_VEX_01_Materials.txt"));  // ③ マテリアルを読む(gentable.py が作る外部ファイル)
//     ...
//     Mv1LikeUnity::Draw(model, mats);                                    // ④ 毎フレーム。メッシュごとに独自シェーダーで描く(MV1DrawModel の代わり)
//     ...
//     Mv1LikeUnity::ReleaseMaterials(model);                              // ⑤ モデルを捨てるとき。③ が読んだ画像とキャッシュを解放する(MV1DeleteModel の直前)
//     MV1DeleteModel(model);
//
// ②③は、そのモデルで(②はプログラム全体で)初めて呼んだときだけ実際に読み込み、以降は呼んでも何もしない(結果をキャッシュして使う)ので、
// ④と一緒に毎フレーム呼んでもよい(②③を呼ぶタイミングを分けたいときだけ、①の前後で個別に呼べばよい)。
// ただし順番は ② → ③ → ④(③は②の結果を見て、独自シェーダー用か標準描画用かの設定をモデルに焼き付けてキャッシュする。
// 確認ツールのように、フォルダの並びでシェーダーの場所を切り替えるなら、その切り替えも②より前に済ませること)。
// ⑤ を忘れても、③ は捨てられたモデルのキャッシュを次の読み込みのときに片づけ、ハンドルの番号が再利用されて別のモデルに
// 当たったときも見分けて読み直す(保険)。ただし画像の解放が遅れるので、モデルを捨てるときは ⑤ を呼ぶこと。
//
// 中身の地図(このヘッダの内部の関数。②〜⑤ が呼ぶ。1 つずつの説明は Mv1LikeUnity_リファレンス.md の「内部の関数」):
//   設定        EnvSettings / Env()(光と出力の設定)・ShaderDir()(シェーダーの場所)
//   ③ の材料    MaterialDef(1 マテリアル分の数値)・ParsedMaterial / ParseMaterialsFile(外部ファイル → セクションごとの key=value)・
//               KvF4 / KvF / KvS / ToT(key=value からの値の取り出し)・BuildMaterialDef(key=value → MaterialDef)・DirOfPath・TEX_SLOTS(画像の枠)
//   ② の中身    ShaderSet / Shaders()(読み込んだシェーダーの置き場)・LoadShaderFiles(.vso .pso を読む)・BuildFgdLut(事前積分表を描く)
//   ③ の結果    Materials / AllMaterials()(モデルごとのキャッシュ)・MaterialsMatch / FreeMaterials(⑤ と、キャッシュの保険)
//   ④ の中身    SetMaterialConstants(定数バッファ b4)・SetViewConstants(定数バッファ b5)。メッシュを描くループ自体は Draw の中
//
// 見た目は、Unity(HDRP 17.3)の作者のマテリアル(ShaderGraph)と光の計算をシェーダーに移植したもの(引き継ぎ資料 20 章。段階 0〜6 で
// Unity と 1/255 程度まで突き合わせてある)。DirectX 11 専用。シェーダー(shaders\bin\)が読めないときは MV1DrawModel に戻る(見た目は別物)。
//
// 光の設定は DxLib の標準ライトと Mv1LikeUnity::Env で与える:
//   - 平行光の向き: DxLib の標準ライト(SetLightDirection / ChangeLightTypeDir)。色 × 強さ: GetLightDifColor() × Env().lightLux(既定 π)
//     → DxLib の既定(白)のとき Unity の「白・π lux」と同じ。
//   - 空(環境光): Env().sky(既定 0.35 = Unity の検証環境の Gradient Sky の色。HDRP は空の露出 0 EV で 1/1.2 を掛けるので、光として届くのは 0.35/1.2)
//   - 露出: Env().exposure(既定 1)。トーンマップは無し(Unity の検証環境と同じ)。
// フォルダの場所を変えるなら、include の前に  #define MV1LIKEUNITY_SHADER_DIR _T("path\\to\\shaders\\bin\\")  を書く。
//
// 制限:
//   - 描画先が 8bit の画面のとき(既定)、まつげ(半透明)は「描く直前の画面の写し」を読んでシェーダーの中でリニアに合成する
//     (GetDrawScreenGraph で毎フレーム画面を 1 回写す)。まつげ同士が重なる所だけ写した時点の値で混ざる(差はわずか)。
//     浮動小数の描画先(MakeScreen + SetDrawValidFloatTypeGraphCreateFlag)に描くなら Env().linearOutput = true にすると、
//     リニアの値をそのまま出し、まつげは事前乗算アルファのブレンドで合成する(最後に自分で sRGB に直すこと)。
//   - 描画のあと、テクスチャのアドレスモードは DX_TEXADDRESS_WRAP(DxLib の既定)に戻す(取得関数が無いため)。
//   - マテリアルの値(色・しきい値・画像のファイル名など)は、モデルごとの外部ファイル(<モデル名>_Materials.txt。gentable.py が
//     Unity のマテリアルから作る。引き継ぎ資料 20・23.3 章)から読む。ファイルに無いマテリアルは DxLib の標準の描画で描く。
//   - 外部ファイル自体が無い・作者の画像が揃っていないモデルは、モデルごと DxLib の標準の描画に戻る
//     (切り抜きだけ設定する。見た目は別物)。SK_VEX_02〜05 は展開するまでこれに当たる(引き継ぎ資料 21 章)。
#pragma once
#include <tchar.h>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <map>

#ifndef MV1LIKEUNITY_SHADER_DIR
#define MV1LIKEUNITY_SHADER_DIR _T("shaders\\bin\\")
#endif

namespace Mv1LikeUnity
{
	typedef std::basic_string<TCHAR> tstring;

	// ---------- 光と出力の設定 ----------
	struct EnvSettings
	{
		float lightLux    = 3.14159265358979f;   // 平行光の強さ(lux)。DxLib の標準ライトの色に掛ける。Unity の検証環境は π
		float sky         = 0.35f;               // 一様な空の色(Unity の Gradient Sky の値)。光として届くのは sky / 1.2(HDRP の空の露出 0 EV)
		float exposure    = 1.0f;                // 露出の倍率
		bool  linearOutput = false;              // true: リニアの値をそのまま出す(浮動小数の描画先向け)。false: sRGB に直して出す(8bit の画面向け)
		float angularDiameter = 0.5f;            // 平行光の見かけの大きさ(度)。HDRP の minRoughness に使う
		bool  disabled = false;                  // true: ④ Draw が何もせず MV1DrawModel で描く(切り分け・比較用。確認ツールの V キー)
	};
	inline EnvSettings& Env() { static EnvSettings e; return e; }
	// シェーダー(shaders\bin\)の場所。実行時に変えるなら ② InitLoadShaders の前に代入する(既定は MV1LIKEUNITY_SHADER_DIR)
	inline tstring& ShaderDir() { static tstring d = MV1LIKEUNITY_SHADER_DIR; return d; }

	// ---------- ③ マテリアルの表(モデルごとの外部ファイル。gentable.py が Unity のマテリアルの値から作る) ----------
	enum { MAT_SKIN = 0, MAT_CLOTHES = 1, MAT_LASHES = 2, MAT_HAIR = 3, MAT_LINGERIE = 4, MAT_TYPE_NUM = 5 };   // MAT_LINGERIE = IDA_Base の切り抜き版(下着。22 章)
	struct MaterialDef
	{
		tstring prefix;           // mv1 のマテリアル名の先頭(MAT_EYES は MAT_EYES_R / _L に当たる。外部ファイルのセクション名)
		int type;                 // MAT_SKIN / MAT_CLOTHES / MAT_LASHES / MAT_HAIR / MAT_LINGERIE(シェーダーの種類)
		int matIdCount;           // MatID の段数
		float p0[4], p1[4], tint[4], add0[4], add1[4], hair0[4], hair1[4], emis[4], micro[4];
		float from[9][4], to[9][4], param[9][4];
	};

	// ---------- ③ 外部ファイル(<モデル名>_Materials.txt)の読み込み ----------
	// 1 マテリアル分(ファイルの1セクション)。kv は key=value の右辺をそのまま文字列で持つ(値の変換は使うときにする)
	struct ParsedMaterial
	{
		tstring prefix;
		std::map<tstring, tstring> kv;
	};

	// UTF-8(バイト)の文字列を TCHAR に変換する。値はすべて ASCII(数値・ファイル名・キー名)なのでこれで問題ない
	// (日本語のコメントは読み飛ばすだけで TCHAR にしない)
	inline tstring ToT(const char* s)
	{
		TCHAR buf[512] = {};
		_stprintf_s(buf, _T("%hs"), s);
		return buf;
	}

	// key=value の右辺(カンマ区切り)を最大 4 個の float に分ける。無ければ既定値のまま
	inline void KvF4(float out[4], const std::map<tstring, tstring>& kv, const TCHAR* key, float d0, float d1, float d2, float d3)
	{
		out[0] = d0; out[1] = d1; out[2] = d2; out[3] = d3;
		std::map<tstring, tstring>::const_iterator it = kv.find(key);
		if (it == kv.end()) return;
		const tstring& s = it->second;
		size_t pos = 0; int i = 0;
		while (i < 4)
		{
			const size_t comma = s.find(_T(','), pos);
			const tstring tok = (comma == tstring::npos) ? s.substr(pos) : s.substr(pos, comma - pos);
			out[i++] = (float)_tstof(tok.c_str());
			if (comma == tstring::npos) break;
			pos = comma + 1;
		}
	}
	inline float KvF(const std::map<tstring, tstring>& kv, const TCHAR* key, float def)
	{
		std::map<tstring, tstring>::const_iterator it = kv.find(key);
		return (it == kv.end()) ? def : (float)_tstof(it->second.c_str());
	}
	inline tstring KvS(const std::map<tstring, tstring>& kv, const TCHAR* key)
	{
		std::map<tstring, tstring>::const_iterator it = kv.find(key);
		return (it == kv.end()) ? tstring() : it->second;
	}

	// <モデル名>_Materials.txt を読んで、セクション(マテリアルの接頭辞)ごとの key=value を集める(引き継ぎ資料 23.3 章)。
	// 行の書式: [接頭辞] / Key=Value(末尾に空白 + ; のコメントがあってもよい)。; で始まる行・空行は無視
	inline std::vector<ParsedMaterial> ParseMaterialsFile(const tstring& path)
	{
		std::vector<ParsedMaterial> out;
		FILE* fp = nullptr;
		if (_tfopen_s(&fp, path.c_str(), _T("rb")) != 0 || fp == nullptr) return out;
		char line[1024];
		ParsedMaterial* cur = nullptr;
		bool first = true;
		while (fgets(line, sizeof(line), fp))
		{
			char* p = line;
			// 手で編集して BOM 付き UTF-8 で保存されても読めるように、先頭の BOM(EF BB BF)は読み飛ばす
			if (first && (unsigned char)p[0] == 0xEF && (unsigned char)p[1] == 0xBB && (unsigned char)p[2] == 0xBF) p += 3;
			first = false;
			while (*p == ' ' || *p == '\t') p++;
			if (*p == ';' || *p == '\r' || *p == '\n' || *p == 0) continue;   // コメント・空行
			if (*p == '[')
			{
				char name[128] = {};
				sscanf_s(p, "[%127[^]]]", name, (unsigned)sizeof(name));
				out.push_back(ParsedMaterial());
				cur = &out.back();
				cur->prefix = ToT(name);
				continue;
			}
			if (cur == nullptr) continue;
			char* eq = strchr(p, '=');
			if (eq == nullptr) continue;
			*eq = 0;
			char* key = p;
			char* val = eq + 1;
			char* semi = strchr(val, ';');
			if (semi != nullptr) *semi = 0;
			// "Key = Value" のように = の前後に空白があっても読めるように、キーの末尾と値の前後の空白を落とす
			char* kend = key + strlen(key);
			while (kend > key && (kend[-1] == ' ' || kend[-1] == '\t')) *--kend = 0;
			while (*val == ' ' || *val == '\t') val++;
			char* end = val + strlen(val);
			while (end > val && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) *--end = 0;
			cur->kv[ToT(key)] = ToT(val);
		}
		fclose(fp);
		return out;
	}

	// セクション 1 個分の key=value から MaterialDef を作る(gentable.py が書き出す形式と対応。型ごとに使わない項目は
	// 元の ShaderGraph の既定値と同じ定数で埋める)
	inline MaterialDef BuildMaterialDef(const ParsedMaterial& sec)
	{
		MaterialDef d = {};
		d.prefix = sec.prefix;
		const std::map<tstring, tstring>& kv = sec.kv;
		const tstring typeStr = KvS(kv, _T("Type"));
		d.type = (typeStr == _T("SKIN")) ? MAT_SKIN : (typeStr == _T("CLOTHES")) ? MAT_CLOTHES
			: (typeStr == _T("LASHES")) ? MAT_LASHES : (typeStr == _T("LINGERIE")) ? MAT_LINGERIE
			: (typeStr == _T("HAIR")) ? MAT_HAIR : -1;   // -1: Type が無い・綴りが違う → そのマテリアルは表に無い扱い(標準描画)
		d.matIdCount = (int)KvF(kv, _T("MatIDCount"), 0.0f);
		const float ao = KvF(kv, _T("AO_Strength"), 0.0f);
		const float metal = KvF(kv, _T("Metallic_Strength"), 0.0f);
		const float smooth = KvF(kv, _T("Smoothness_Strength"), 0.0f);
		switch (d.type)
		{
		case MAT_SKIN:
			d.p0[0] = ao; d.p0[1] = metal; d.p0[2] = smooth; d.p0[3] = KvF(kv, _T("Normal_Blending"), 0.0f);
			d.p1[0] = KvF(kv, _T("BaseColor_ScaleByCenter"), 0.0f); d.p1[1] = KvF(kv, _T("Additive_UseLerp"), 0.0f); d.p1[2] = 0.0f; d.p1[3] = KvF(kv, _T("Additive_MicrodetailOn"), 0.0f);
			KvF4(d.tint, kv, _T("Additive_Tint"), 1, 1, 1, 1);
			{
				float gs[4]; KvF4(gs, kv, _T("Additive_GridSize"), 1, 1, 0, 0);
				d.add0[0] = gs[0]; d.add0[1] = gs[1]; d.add0[2] = KvF(kv, _T("Additive_TileNumber"), 0.0f); d.add0[3] = KvF(kv, _T("Additive_Scale"), 0.0f);
			}
			d.add1[0] = KvF(kv, _T("Additive_Rotation"), 0.0f); d.add1[1] = KvF(kv, _T("Additive_OffsetX"), 0.0f); d.add1[2] = KvF(kv, _T("Additive_OffsetY"), 0.0f); d.add1[3] = KvF(kv, _T("Additive_NormalStrength"), 0.0f);
			KvF4(d.emis, kv, _T("Emissive_Color"), 0, 0, 0, 0);
			KvF4(d.hair0, kv, _T("__unused__"), 0, 0, 0, 0); KvF4(d.hair1, kv, _T("__unused__"), 0, 0, 0, 0);
			d.micro[0] = KvF(kv, _T("Microdetail_Strength"), 0.0f); d.micro[1] = KvF(kv, _T("Microdetail_Size"), 0.0f); d.micro[2] = 0.0f; d.micro[3] = 0.0f;
			break;
		case MAT_CLOTHES:
			d.p0[0] = ao; d.p0[1] = metal; d.p0[2] = smooth; d.p0[3] = KvF(kv, _T("Normal_Strength"), 0.0f);
			d.p1[0] = 1.0f; d.p1[1] = 0.0f; d.p1[2] = KvF(kv, _T("Alpha_Clip_Threshold"), 0.0f); d.p1[3] = 0.0f;
			d.tint[0] = d.tint[1] = d.tint[2] = d.tint[3] = 1.0f;
			d.add0[0] = d.add0[1] = d.add0[2] = d.add0[3] = 1.0f; d.add1[0] = d.add1[1] = d.add1[2] = d.add1[3] = 0.0f;
			KvF4(d.emis, kv, _T("Emission_Color"), 0, 0, 0, 0);
			d.hair0[0] = d.hair0[1] = d.hair0[2] = d.hair0[3] = 0.0f; d.hair1[0] = d.hair1[1] = d.hair1[2] = d.hair1[3] = 0.0f;
			d.micro[0] = KvF(kv, _T("Microdetail_Strength"), 0.0f); d.micro[1] = KvF(kv, _T("Microdetail_Size"), 0.0f); d.micro[2] = 0.0f; d.micro[3] = 0.0f;
			break;
		case MAT_LASHES:
		case MAT_LINGERIE:
			d.p0[0] = ao; d.p0[1] = metal; d.p0[2] = smooth; d.p0[3] = KvF(kv, _T("Normal_Strength"), 0.0f);
			d.p1[0] = 1.0f; d.p1[1] = 0.0f; d.p1[2] = KvF(kv, _T("Alpha_Threshold"), 0.0f); d.p1[3] = 0.0f;
			KvF4(d.tint, kv, _T("BaseColor_Tint"), 1, 1, 1, 1);
			d.add0[0] = d.add0[1] = d.add0[2] = d.add0[3] = 1.0f; d.add1[0] = d.add1[1] = d.add1[2] = d.add1[3] = 0.0f;
			KvF4(d.emis, kv, _T("Emissive_Tint"), 0, 0, 0, 0);
			d.hair0[0] = d.hair0[1] = d.hair0[2] = d.hair0[3] = 0.0f; d.hair1[0] = d.hair1[1] = d.hair1[2] = d.hair1[3] = 0.0f;
			d.micro[0] = d.micro[1] = d.micro[2] = d.micro[3] = 0.0f;
			break;
		case MAT_HAIR:
		default:
			d.p0[0] = ao; d.p0[1] = metal; d.p0[2] = smooth; d.p0[3] = KvF(kv, _T("Normal_Strength"), 0.0f);
			d.p1[0] = 1.0f; d.p1[1] = 0.0f; d.p1[2] = KvF(kv, _T("Alpha_Threshold"), 0.0f); d.p1[3] = 0.0f;
			d.tint[0] = d.tint[1] = d.tint[2] = d.tint[3] = 1.0f;
			d.add0[0] = d.add0[1] = d.add0[2] = d.add0[3] = 1.0f; d.add1[0] = d.add1[1] = d.add1[2] = d.add1[3] = 0.0f;
			d.emis[0] = d.emis[1] = d.emis[2] = d.emis[3] = 0.0f;
			{
				float r[4], t[4];
				KvF4(r, kv, _T("Root_Color"), 0, 0, 0, 0); KvF4(t, kv, _T("Tip_Color"), 0, 0, 0, 0);
				d.hair0[0] = r[0]; d.hair0[1] = r[1]; d.hair0[2] = r[2]; d.hair0[3] = KvF(kv, _T("Noise_Tiling"), 0.0f);
				d.hair1[0] = t[0]; d.hair1[1] = t[1]; d.hair1[2] = t[2]; d.hair1[3] = 0.0f;
			}
			d.micro[0] = d.micro[1] = d.micro[2] = d.micro[3] = 0.0f;
			break;
		}
		for (int i = 0; i < 9; i++)
		{
			if (i < d.matIdCount)
			{
				TCHAR key[40];
				_stprintf_s(key, _T("MatID%d_From"), i + 1); KvF4(d.from[i], kv, key, 0, 0, 0, 0);
				_stprintf_s(key, _T("MatID%d_To"), i + 1);   KvF4(d.to[i], kv, key, 0, 0, 0, 0);
				_stprintf_s(key, _T("MatID%d_Expand"), i + 1);          d.param[i][0] = KvF(kv, key, 0.0f);
				_stprintf_s(key, _T("MatID%d_Blur"), i + 1);            d.param[i][1] = KvF(kv, key, 0.0f);
				_stprintf_s(key, _T("MatID%d_PatternIntensity"), i + 1); d.param[i][2] = KvF(kv, key, 0.0f);
				d.param[i][3] = 0.0f;
			}
			else
			{
				d.from[i][0] = d.from[i][1] = d.from[i][2] = d.from[i][3] = 0.0f;
				d.to[i][0] = d.to[i][1] = d.to[i][2] = d.to[i][3] = 0.0f;
				d.param[i][0] = d.param[i][1] = d.param[i][2] = d.param[i][3] = 0.0f;
			}
		}
		return d;
	}

	// モデルのフォルダ(末尾に / か \)からモデル名を取る: "Runtime/Models/SK_VEX_02/" → "SK_VEX_02"(サムネイル表示等に使う)
	inline tstring ModelNameFromDir(const tstring& dir)
	{
		tstring s = dir;
		while (!s.empty() && (s[s.size() - 1] == _T('/') || s[s.size() - 1] == _T('\\'))) s.erase(s.size() - 1);
		const size_t p = s.find_last_of(_T("/\\"));
		return (p == tstring::npos) ? s : s.substr(p + 1);
	}
	// ファイルパスからフォルダの部分を取る(末尾に / か \ を残す): "SK_VEX_01/SK_VEX_01_Materials.txt" → "SK_VEX_01/"
	inline tstring DirOfPath(const tstring& path)
	{
		const size_t p = path.find_last_of(_T("/\\"));
		return (p == tstring::npos) ? tstring() : path.substr(0, p + 1);
	}

	// 作者の画像の枠番号(IdaFaber_Surface.hlsli のテクスチャレジスタ t<N> にそのまま対応。0・1・2・3・8・9・10・11・12・13・14 だけ使う)
	static const int TEX_SLOTS[] = { 0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14 };
	static const int TEX_SLOT_NUM = 11;


	// ---------- ② シェーダー ----------
	// ② の結果の置き場(プログラム全体で 1 つ。Shaders() で取る): 読み込んだ頂点・ピクセルシェーダーのハンドル、定数バッファ、事前積分表
	struct ShaderSet
	{
		int vs[DX_MV1_VERTEX_TYPE_NUM] = {};
		int ps[MAT_TYPE_NUM] = { -1, -1, -1, -1, -1 };        // sRGB 出力(PS_IDA_*.pso)
		int psLinear[MAT_TYPE_NUM] = { -1, -1, -1, -1, -1 };  // リニア出力(PS_IDA_*_LINEAR.pso)
		int psFgd = -1;                                   // 事前積分の表を作る 2D 用
		int cbMaterial = -1, cbView = -1;                 // 定数バッファ b4 / b5
		int fgdLut = -1;                                  // 事前積分の表(64×64 浮動小数。起動時に描く)
		int screenCopy = -1, screenW = 0, screenH = 0;    // まつげ用の画面の写し(8bit の画面のとき)
		bool tried = false, ok = false;
	};
	inline ShaderSet& Shaders() { static ShaderSet s; return s; }

	// .pso(独自ピクセルシェーダー)・.vso(頂点シェーダー)を読み込む(② InitLoadShaders の内部実装。事前積分表は別)
	inline void LoadShaderFiles()
	{
		ShaderSet& g = Shaders();
		if (g.tried) return;
		g.tried = true;
		const tstring dir = ShaderDir();
		const int vsRigid     = LoadVertexShader((dir + _T("VS_RIGID.vso")).c_str());
		const int vsNmapRigid = LoadVertexShader((dir + _T("VS_NMAPRIGID.vso")).c_str());
		g.vs[DX_MV1_VERTEX_TYPE_1FRAME]          = vsRigid;
		g.vs[DX_MV1_VERTEX_TYPE_4FRAME]          = LoadVertexShader((dir + _T("VS_SKIN4.vso")).c_str());
		g.vs[DX_MV1_VERTEX_TYPE_8FRAME]          = LoadVertexShader((dir + _T("VS_SKIN8.vso")).c_str());
		g.vs[DX_MV1_VERTEX_TYPE_FREE_FRAME]      = vsRigid;
		g.vs[DX_MV1_VERTEX_TYPE_NMAP_1FRAME]     = vsNmapRigid;
		g.vs[DX_MV1_VERTEX_TYPE_NMAP_4FRAME]     = LoadVertexShader((dir + _T("VS_NMAP4.vso")).c_str());
		g.vs[DX_MV1_VERTEX_TYPE_NMAP_8FRAME]     = LoadVertexShader((dir + _T("VS_NMAP8.vso")).c_str());
		g.vs[DX_MV1_VERTEX_TYPE_NMAP_FREE_FRAME] = vsNmapRigid;
		const TCHAR* kinds[MAT_TYPE_NUM] = { _T("SKIN"), _T("CLOTHES"), _T("LASHES"), _T("HAIR"), _T("LINGERIE") };
		bool ok = true;
		for (int k = 0; k < MAT_TYPE_NUM; k++)
		{
			g.ps[k]       = LoadPixelShader((dir + _T("PS_IDA_") + kinds[k] + _T(".pso")).c_str());
			g.psLinear[k] = LoadPixelShader((dir + _T("PS_IDA_") + kinds[k] + _T("_LINEAR.pso")).c_str());
			if (g.ps[k] == -1 || g.psLinear[k] == -1) ok = false;
		}
		g.psFgd = LoadPixelShader((dir + _T("PS_FGD.pso")).c_str());
		if (g.psFgd == -1) ok = false;
		for (int i = 0; i < DX_MV1_VERTEX_TYPE_NUM; i++) if (g.vs[i] == -1) ok = false;
		if (ok)
		{
			g.cbMaterial = CreateShaderConstantBuffer(sizeof(FLOAT4) * 37);
			g.cbView     = CreateShaderConstantBuffer(sizeof(FLOAT4) * 6);
			if (g.cbMaterial == -1 || g.cbView == -1) ok = false;
		}
		g.ok = ok;
	}

	// HDRP の事前積分の表(_PreIntegratedFGD_GGXDisneyDiffuse。64×64)を、浮動小数の描画先の画像に 1 回描く(Mv1LikeUnity_FGD_PS.hlsl)。
	// ② InitLoadShaders の内部実装(LoadShaderFiles のあとに呼ぶ。2 回目以降は何もしない)。
	// SetDrawScreen でカメラの設定が消えるので、カメラと描画先を保存して戻す。DxLib_Init の直後に呼ぶのが安全
	inline void BuildFgdLut()
	{
		ShaderSet& g = Shaders();
		if (!g.ok || g.fgdLut != -1) return;
		const int screen = GetDrawScreen();
		const MATRIX view = GetCameraViewMatrix();
		const MATRIX proj = GetCameraProjectionMatrix();
		SetDrawValidGraphCreateFlag(TRUE);
		SetDrawValidFloatTypeGraphCreateFlag(TRUE);
		SetCreateDrawValidGraphChannelNum(4);
		SetCreateGraphChannelBitDepth(32);
		g.fgdLut = MakeScreen(64, 64, TRUE);
		SetCreateGraphChannelBitDepth(0);
		SetDrawValidFloatTypeGraphCreateFlag(FALSE);
		SetDrawValidGraphCreateFlag(FALSE);
		if (g.fgdLut == -1) { g.ok = false; return; }
		SetDrawScreen(g.fgdLut);
		ClearDrawScreen();
		SetUseVertexShader(-1);
		SetUsePixelShader(g.psFgd);
		VERTEX2DSHADER v[6];
		const float xy[6][2] = { {0, 0}, {64, 0}, {0, 64}, {64, 0}, {64, 64}, {0, 64} };
		for (int i = 0; i < 6; i++)
		{
			v[i].pos = VGet(xy[i][0], xy[i][1], 0.0f);
			v[i].rhw = 1.0f;
			v[i].dif = GetColorU8(255, 255, 255, 255);
			v[i].spc = GetColorU8(0, 0, 0, 0);
			v[i].u = xy[i][0] / 64.0f; v[i].v = xy[i][1] / 64.0f;
			v[i].su = v[i].sv = 0.0f;
		}
		DrawPrimitive2DToShader(v, 6, DX_PRIMTYPE_TRIANGLELIST);
		SetUsePixelShader(-1);
		SetDrawScreen(screen);
		SetCameraViewMatrix(view);
		SetupCamera_ProjectionMatrix(proj);
	}

	// ---------- ③ マテリアルの読み込み結果(モデルごと) ----------
	struct Materials
	{
		std::vector<MaterialDef> owned;   // 外部ファイルから作った表の行の実体(def はここを指す。読み込み後は追加しない)
		std::vector<int> tex;       // [マテリアル番号 × 16 + 枠] のグラフィックハンドル(-1 = 渡さない)
		std::vector<const MaterialDef*> def;   // マテリアル番号 → 表の行(nullptr = 外部ファイルに無い)
		std::vector<bool> lashes;   // マテリアル番号 → まつげ(半透明)か
		std::vector<unsigned int> meshBit;     // メッシュ番号 → 両面表示の指定ビット(MV1LIKEUNITY_TWOSIDED_*。0 = 常に両面)
		unsigned int twoSided = 0xffffffffu;   // 前回の指定(変わったときだけ設定し直す)
		bool ok = true;                        // false: 外部ファイルが無い・作者の画像が揃っていない。DxLib の標準描画に戻す
		int missing = 0;                       // 読めなかった画像の数(外部ファイル自体が無ければ 1)
		std::vector<int> graphs;               // ③ がファイルから読み込んだ画像のハンドル(重複なし。⑤ ReleaseMaterials で DeleteGraph する)
		// このキャッシュがどのモデルのものかの印(ハンドルの番号が再利用されて別のモデルに当たったことを見分ける)
		int sigMaterialNum = -1, sigMeshNum = -1, sigTexture0 = -1;
	};
	// ③ の結果の置き場(mv1 のハンドル → Materials。プログラム全体で 1 つ)。③ が入れ、④ が読み、⑤ が消す
	inline std::map<int, Materials>& AllMaterials() { static std::map<int, Materials> m; return m; }

	// ③ の結果が今の model のものか(mv1 のマテリアル数・メッシュ数・最初のテクスチャのグラフィックハンドルで見分ける。
	// DxLib のハンドルは番号に「何回目に作ったか」が埋め込まれているので、読み直したモデルの値はまず一致しない)
	inline bool MaterialsMatch(int model, const Materials& st)
	{
		return MV1GetMaterialNum(model) == st.sigMaterialNum && MV1GetMeshNum(model) == st.sigMeshNum &&
			(MV1GetTextureNum(model) > 0 ? MV1GetTextureGraphHandle(model, 0) : -1) == st.sigTexture0;
	}
	// ③ が読み込んだ画像を解放して、結果を空に戻す(map の要素そのものは残すので、呼び出し側が持つ参照は生きたまま)
	inline void FreeMaterials(Materials& st)
	{
		for (size_t i = 0; i < st.graphs.size(); i++) DeleteGraph(st.graphs[i]);
		st = Materials();
	}
	// ⑤ モデルを捨てるとき(MV1DeleteModel の直前)に呼ぶ: ③ が読んだ画像とキャッシュを解放する。
	// 呼び出し側が ③ の戻り値の参照を持っていたら、これ以降は使わないこと。
	inline void ReleaseMaterials(int model)
	{
		std::map<int, Materials>& models = AllMaterials();
		std::map<int, Materials>::iterator it = models.find(model);
		if (it == models.end()) return;
		FreeMaterials(it->second);
		models.erase(it);
	}

	// マテリアル名(完全一致)からマテリアル番号を探す(無ければ -1)。呼び出し側向けの補助(ヘッダの中では使っていない)
	inline int FindMaterial(int model, const TCHAR* name)
	{
		const int num = MV1GetMaterialNum(model);
		for (int i = 0; i < num; i++) if (_tcscmp(MV1GetMaterialName(model, i), name) == 0) return i;
		return -1;
	}

	// ③ マテリアルを読む。materialsPath は <モデル名>_Materials.txt のパス(mv1 とテクスチャは同じフォルダにある前提)。
	// そのモデルを初めて渡したときだけ実際に読み込み(外部ファイルの解析・作者の画像をファイルから)、
	// 以降は結果をキャッシュして返すだけ(毎フレーム呼んでよい)。
	// キャッシュは mv1 のハンドルで引く。⑤ ReleaseMaterials を呼ばずに MV1DeleteModel したモデルの分は、次に新しいモデルを
	// 読むときに片づける(ハンドルが無効になったもの)。ハンドルの番号が再利用されて別のモデルに当たったときは印で見分けて読み直す。
	inline Materials& LoadMaterials(int model, const TCHAR* materialsPath)
	{
		const tstring path = materialsPath;
		const tstring dir = DirOfPath(path);
		std::map<int, Materials>& models = AllMaterials();
		std::map<int, Materials>::iterator it = models.find(model);
		if (it != models.end())
		{
			if (MaterialsMatch(model, it->second)) return it->second;
			FreeMaterials(it->second);   // 同じ番号だが別のモデル(番号が再利用された): 古い画像を捨てて読み直す。参照は生きたまま
		}
		else
		{
			// 新しいモデル: 捨てられたモデル(ハンドルが無効)のキャッシュを片づける(⑤ を忘れたときの保険)
			for (std::map<int, Materials>::iterator d = models.begin(); d != models.end(); )
			{
				if (MV1GetMaterialNum(d->first) == -1) { FreeMaterials(d->second); d = models.erase(d); }
				else ++d;
			}
		}
		Materials& st = models[model];
		const int matNum = MV1GetMaterialNum(model);
		st.tex.assign(matNum * 16, -1);
		st.def.assign(matNum, nullptr);
		st.lashes.assign(matNum, false);
		// Unity の作者のマテリアルはすべて両面表示(髪・まつげ・下着だけ ④ Draw の引数で片面にできる)
		st.meshBit.assign(MV1GetMeshNum(model), 0u);
		for (int i = 0; i < MV1GetMeshNum(model); i++)
		{
			const int mat = MV1GetMeshMaterial(model, i);
			const TCHAR* n = (mat != -1) ? MV1GetMaterialName(model, mat) : _T("");
			if      (_tcsncmp(n, _T("MAT_HAIR"), 8) == 0)     st.meshBit[i] = 1u;
			else if (_tcsncmp(n, _T("MAT_LASHES"), 10) == 0)  st.meshBit[i] = 2u;
			else if (_tcsncmp(n, _T("MAT_LINGERIE"), 12) == 0) st.meshBit[i] = 4u;
			MV1SetMeshBackCulling(model, i, FALSE);
		}
		// 外部ファイルを読む(セクション = マテリアルの接頭辞ごとに 1 つ)。読めなければ st.missing で標準描画に戻す。
		// ② のシェーダーが読めていない(未実施・失敗)ときも同じ扱い: 独自シェーダー前提の設定(切り抜きを切る等)をせず、標準描画用の設定にする
		const std::vector<ParsedMaterial> sections = ParseMaterialsFile(path);
		if (sections.empty() || !Shaders().ok) st.missing++;
		st.owned.reserve(sections.size());
		for (size_t i = 0; i < sections.size(); i++) st.owned.push_back(BuildMaterialDef(sections[i]));   // 先に全部作る(あとでポインタを取るため)

		std::map<tstring, int> loaded;
		for (int m = 0; m < matNum; m++)
		{
			const TCHAR* name = MV1GetMaterialName(model, m);
			st.lashes[m] = _tcsncmp(name, _T("MAT_LASHES"), 10) == 0;
			// 外部ファイルのセクションのうち、名前がその接頭辞で始まる最初のもの(MAT_EYES_R/_L は MAT_EYES に一致)
			size_t si = sections.size();
			for (size_t i = 0; i < sections.size(); i++)
				if (_tcsncmp(name, sections[i].prefix.c_str(), sections[i].prefix.size()) == 0) { si = i; break; }
			if (si == sections.size()) continue;   // 対応するセクションが無い(表に無いマテリアル)
			if (st.owned[si].type < 0) continue;   // Type が読めないセクション(手で編集して綴りを間違えた等)も表に無い扱い
			st.def[m] = &st.owned[si];
			const std::map<tstring, tstring>& kv = sections[si].kv;
			for (int s = 0; s < TEX_SLOT_NUM; s++)
			{
				const int slot = TEX_SLOTS[s];
				TCHAR key[16]; _stprintf_s(key, _T("Tex%d"), slot);
				const tstring file = KvS(kv, key);
				if (file.empty()) continue;
				const tstring path2 = dir + file;
				// MV1LoadTexture は無いファイルでも失敗せず、白い既定のテクスチャのハンドルを返す。先にファイルの有無を確かめる
				if (GetFileAttributes(path2.c_str()) == INVALID_FILE_ATTRIBUTES)
				{
					st.missing++;
					continue;
				}
				if (slot == 0)
				{
					// 髪の色画像: mv1 が作者の画像(T_HAIR_STRAIGHT_albedo.png)を参照していればそのまま。古い mv1(焼き込み版・詰め合わせ版)なら差し替える
					const int tex = MV1GetMaterialDifMapTexture(model, m);
					const TCHAR* cur = (tex >= 0) ? MV1GetTextureColorFilePath(model, tex) : nullptr;
					const bool same = cur != nullptr && _tcslen(cur) >= file.size() && _tcscmp(cur + _tcslen(cur) - file.size(), file.c_str()) == 0;
					if (!same && tex >= 0)
					{
						std::map<tstring, int>::iterator f = loaded.find(path2);
						int gh;
						if (f != loaded.end()) gh = f->second;
						else { gh = MV1LoadTexture(path2.c_str()); loaded[path2] = gh; if (gh != -1) st.graphs.push_back(gh); }
						if (gh != -1) MV1SetTextureGraphHandle(model, tex, gh, FALSE);   // mv1 を捨てても DxLib はこの画像を解放しない(⑤ で解放する)
						else st.missing++;
					}
					continue;
				}
				std::map<tstring, int>::iterator f = loaded.find(path2);
				int gh;
				if (f != loaded.end()) gh = f->second;
				else { gh = MV1LoadTexture(path2.c_str()); loaded[path2] = gh; if (gh != -1) st.graphs.push_back(gh); }   // MV1LoadTexture: 全段のミップマップ付き
				if (gh == -1) st.missing++;
				st.tex[m * 16 + slot] = gh;
			}
		}
		if (st.missing > 0)
		{
			// 作者の画像が揃っていない(SK_VEX_02〜05 など、まだ展開していないモデル): DxLib の標準描画に戻す。
			// 標準描画で最低限の見た目になるよう、切り抜き(髪 79・まつげ 18・下着 76。以前の IdaFaber.h と同じ値)だけ設定する
			st.ok = false;
			for (int m = 0; m < matNum; m++)
			{
				const TCHAR* name = MV1GetMaterialName(model, m);
				if      (_tcsncmp(name, _T("MAT_HAIR"), 8) == 0)      MV1SetMaterialDrawAlphaTest(model, m, TRUE, DX_CMP_GREATER, 79);
				else if (_tcsncmp(name, _T("MAT_LASHES"), 10) == 0)   MV1SetMaterialDrawAlphaTest(model, m, TRUE, DX_CMP_GREATER, 18);
				else if (_tcsncmp(name, _T("MAT_LINGERIE"), 12) == 0) MV1SetMaterialDrawAlphaTest(model, m, TRUE, DX_CMP_GREATER, 76);
			}
			// 標準描画では髪・まつげ・下着以外を片面に戻す
			for (size_t i = 0; i < st.meshBit.size(); i++) if (st.meshBit[i] == 0u) MV1SetMeshBackCulling(model, (int)i, TRUE);
		}
		else
		{
			// 独自シェーダーで描く: シェーダーが自分で切り抜くので、mv1 のアルファテストとブレンドは切る(まつげは描くときに合成する)
			for (int m = 0; m < matNum; m++)
			{
				if (st.def[m] == nullptr) continue;
				MV1SetMaterialDrawAlphaTest(model, m, FALSE, DX_CMP_GREATER, 0);
				MV1SetMaterialDrawBlendMode(model, m, DX_BLENDMODE_NOBLEND);
			}
		}
		// このモデルの印(髪の画像を差し替えたあとで取る。MaterialsMatch はこれと比べる)
		st.sigMaterialNum = matNum;
		st.sigMeshNum = MV1GetMeshNum(model);
		st.sigTexture0 = MV1GetTextureNum(model) > 0 ? MV1GetTextureGraphHandle(model, 0) : -1;
		return st;
	}

	// ② シェーダーの準備(DxLib_Init の直後に 1 回呼ぶ。2 回目以降に呼んでも何もしない):
	//   1. shaders\bin\*.pso(独自ピクセルシェーダー)・*.vso(頂点シェーダー)を読み込む(LoadShaderFiles)
	//   2. 光の計算で使う事前積分表(FGD LUT。64×64 のテクスチャ)を、専用のシェーダーで 1 回描いて作る(BuildFgdLut)。
	//      ファイルではなく GPU の計算で作る。全モデル・全マテリアル共通で 1 個だけなので、③④ と違って独立した呼び出しにはしていない
	// ③④ より前に呼ぶこと(④ Draw は自動では呼ばない。呼んでいなければ Draw は標準描画 MV1DrawModel に戻る)。
	// 事前積分表の作成でカメラの設定を一時的に戻すので、DxLib_Init の直後(カメラを設定する前)が安全。
	// 戻り値: 両方読み込めたか(false ならシェーダーが使えず、③④ は標準描画に戻る)
	inline bool InitLoadShaders()
	{
		LoadShaderFiles();
		BuildFgdLut();
		return Shaders().ok;
	}

	// ④ の内部: 定数バッファ b4(マテリアルの値。float4 × 37)に MaterialDef を書いて、ピクセルシェーダーの b4 に付ける。
	// 三角形リストを描く直前に、そのマテリアルの分を呼ぶ。並びはシェーダー側(IdaFaber_Surface.hlsli の cbuffer)と同じ:
	//   [0..8] p0, p1, tint, add0, add1, hair0, hair1, emis, micro / [9].x MatID の段数 / [10..18] MatID の From / [19..27] To / [28..36] Expand・Blur・PatternIntensity
	inline void SetMaterialConstants(const MaterialDef* d)
	{
		ShaderSet& g = Shaders();
		FLOAT4* c = (FLOAT4*)GetBufferShaderConstantBuffer(g.cbMaterial);
		const float* rows[9] = { d->p0, d->p1, d->tint, d->add0, d->add1, d->hair0, d->hair1, d->emis, d->micro };
		for (int r = 0; r < 9; r++) { c[r].x = rows[r][0]; c[r].y = rows[r][1]; c[r].z = rows[r][2]; c[r].w = rows[r][3]; }
		c[9].x = (float)d->matIdCount; c[9].y = c[9].z = c[9].w = 0.0f;
		for (int i = 0; i < 9; i++)
		{
			c[10 + i].x = d->from[i][0];  c[10 + i].y = d->from[i][1];  c[10 + i].z = d->from[i][2];  c[10 + i].w = d->from[i][3];
			c[19 + i].x = d->to[i][0];    c[19 + i].y = d->to[i][1];    c[19 + i].z = d->to[i][2];    c[19 + i].w = d->to[i][3];
			c[28 + i].x = d->param[i][0]; c[28 + i].y = d->param[i][1]; c[28 + i].z = d->param[i][2]; c[28 + i].w = d->param[i][3];
		}
		UpdateShaderConstantBuffer(g.cbMaterial);
		SetShaderConstantBuffer(g.cbMaterial, DX_SHADERTYPE_PIXEL, 4);
	}

	// ④ の内部: 定数バッファ b5(カメラと光。float4 × 6)を書いて、ピクセルシェーダーの b5 に付ける。毎フレーム描く前に 1 回呼ぶ。
	//   [0..2] ビュー行列の 3 行 / [3] 光の来る向き(ビュー空間)と .w = HDRP の minRoughness / [4] 光の色 × Env().lightLux / [5] 空 Env().sky / 1.2 と .w = 露出
	// 光の向きと色は DxLib の標準ライト(GetLightDirection / GetLightDifColor)から取る
	inline void SetViewConstants()
	{
		ShaderSet& g = Shaders();
		const EnvSettings& e = Env();
		const MATRIX v = GetCameraViewMatrix();
		FLOAT4* c = (FLOAT4*)GetBufferShaderConstantBuffer(g.cbView);
		for (int i = 0; i < 3; i++) { c[i].x = v.m[i][0]; c[i].y = v.m[i][1]; c[i].z = v.m[i][2]; c[i].w = 0.0f; }
		// 光の来る向き(ビュー空間)。w = HDRP の minRoughness(平行光の角直径から。HDGpuLightsBuilder の式)
		const VECTOR lw = VScale(GetLightDirection(), -1.0f);
		const VECTOR lv = VNorm(VTransformSR(lw, v));
		const float t = 1.15f * (0.0315f * e.angularDiameter + 0.4f);
		float maxSmoothness = 1.35f / (1.0f + t * t) - 0.11f;
		if (maxSmoothness < 0.0f) maxSmoothness = 0.0f;
		if (maxSmoothness > 1.0f) maxSmoothness = 1.0f;
		c[3].x = lv.x; c[3].y = lv.y; c[3].z = lv.z; c[3].w = (1.0f - maxSmoothness) * (1.0f - maxSmoothness);
		const COLOR_F lc = GetLightDifColor();
		c[4].x = lc.r * e.lightLux; c[4].y = lc.g * e.lightLux; c[4].z = lc.b * e.lightLux; c[4].w = 0.0f;
		c[5].x = c[5].y = c[5].z = e.sky / 1.2f; c[5].w = e.exposure;
		UpdateShaderConstantBuffer(g.cbView);
		SetShaderConstantBuffer(g.cbView, DX_SHADERTYPE_PIXEL, 5);
	}

	// ④ Draw の twoSided 引数(両面表示にするマテリアル。足し合わせて指定する。既定は全部)。
	// Unity の作者のマテリアルはすべて両面。髪・まつげ・下着以外は常に両面
#define MV1LIKEUNITY_TWOSIDED_NONE     0u
#define MV1LIKEUNITY_TWOSIDED_HAIR     1u
#define MV1LIKEUNITY_TWOSIDED_LASHES   2u
#define MV1LIKEUNITY_TWOSIDED_LINGERIE 4u
#define MV1LIKEUNITY_TWOSIDED_ALL      7u

	// ④ メッシュごとに独自シェーダーで描く(MV1DrawModel の代わり。毎フレーム呼ぶ)。materials は ③ LoadMaterials の戻り値。
	// シェーダーが読めていない(② 未実施・失敗)か、作者の画像が揃っていない(③ の materials.ok = false)か、
	// Env().disabled(確認ツールの V キー)のときは、何もせず標準描画 MV1DrawModel に戻る(見た目は別物になる)。
	inline void Draw(int model, Materials& materials, unsigned int twoSided = MV1LIKEUNITY_TWOSIDED_ALL)
	{
		Materials& st = materials;
		if (Env().disabled || !Shaders().ok) { MV1DrawModel(model); return; }
		ShaderSet& g = Shaders();
		const EnvSettings& e = Env();
		if (st.twoSided != twoSided)
		{
			st.twoSided = twoSided;
			for (size_t i = 0; i < st.meshBit.size(); i++)
			{
				if (st.meshBit[i] == 0u) MV1SetMeshBackCulling(model, (int)i, st.ok ? FALSE : TRUE);   // 独自シェーダーなら全部両面(Unity と同じ)、標準描画なら片面
				else MV1SetMeshBackCulling(model, (int)i, (twoSided & st.meshBit[i]) == 0u ? TRUE : FALSE);
			}
		}
		if (!st.ok) { MV1DrawModel(model); return; }   // 作者の画像が揃っていないモデルは標準描画(見た目は別物)
		// まつげ用: 8bit の画面のときは、不透明なものを描いた後で画面を写し、まつげのシェーダーがそれを読んでリニアに合成する
		int screenW = 0, screenH = 0;
		GetDrawScreenSize(&screenW, &screenH);
		if (!e.linearOutput && (g.screenCopy == -1 || g.screenW != screenW || g.screenH != screenH))
		{
			if (g.screenCopy != -1) DeleteGraph(g.screenCopy);
			g.screenCopy = MakeGraph(screenW, screenH);
			g.screenW = screenW; g.screenH = screenH;
		}

		// サンプラー(枠 1 以降は全体の設定が使われる): Unity の品質設定と同じ異方性 16・繰り返し
		const int savedDrawMode = GetDrawMode();
		const int savedAniso = GetMaxAnisotropy();
		SetDrawMode(DX_DRAWMODE_ANISOTROPIC);
		SetMaxAnisotropy(16);
		SetTextureAddressMode(DX_TEXADDRESS_WRAP);
		SetViewConstants();
		SetUseTextureToShader(15, g.fgdLut);

		MV1SetUseOrigShader(TRUE);
		const int num = MV1GetTriangleListNum(model);
		// pass 0: 不透明なもの、pass 1: まつげ(半透明。Z 書き込みなし。浮動小数の描画先なら事前乗算アルファ、8bit の画面なら写しを読んで合成)
		for (int pass = 0; pass < 2; pass++)
		for (int t = 0; t < num; t++)
		{
			if (pass == 1 && t == 0 && !e.linearOutput && g.screenCopy != -1)
			{
				GetDrawScreenGraph(0, 0, screenW, screenH, g.screenCopy);
				SetUseTextureToShader(7, g.screenCopy);
			}
			const int mat = MV1GetTriangleListUseMaterial(model, t);
			const MaterialDef* d = (mat != -1) ? st.def[mat] : nullptr;
			const bool lashes = (mat != -1) && st.lashes[mat];
			if (lashes != (pass == 1)) continue;
			if (d == nullptr)
			{
				// 表に無いマテリアルは DxLib の標準の描画
				MV1SetUseOrigShader(FALSE);
				MV1DrawTriangleList(model, t);
				MV1SetUseOrigShader(TRUE);
				continue;
			}
			const int vt = MV1GetTriangleListVertexType(model, t);
			const int vs = (vt >= 0 && vt < DX_MV1_VERTEX_TYPE_NUM) ? g.vs[vt] : -1;
			const int ps = e.linearOutput ? g.psLinear[d->type] : g.ps[d->type];
			if (vs == -1 || ps == -1) continue;
			for (int slot = 1; slot < 15; slot++)
			{
				if (slot >= 4 && slot <= 7) continue;   // 4〜6 はモデルの画像のまま、7 はまつげ用の画面の写し
				SetUseTextureToShader(slot, st.tex[mat * 16 + slot]);
			}
			SetMaterialConstants(d);
			if (lashes)
			{
				MV1SetMaterialDrawBlendMode(model, mat, e.linearOutput ? DX_BLENDMODE_PMA_ALPHA : DX_BLENDMODE_NOBLEND);
				MV1SetMaterialDrawBlendParam(model, mat, 255);
				MV1SetWriteZBuffer(model, FALSE);
			}
			SetUseVertexShader(vs);
			SetUsePixelShader(ps);
			MV1DrawTriangleList(model, t);
			if (lashes)
			{
				MV1SetWriteZBuffer(model, TRUE);
				MV1SetMaterialDrawBlendMode(model, mat, DX_BLENDMODE_NOBLEND);
			}
		}
		MV1SetUseOrigShader(FALSE);
		// このあとの描画(線・文字・2D のシェーダー描画)に残らないように全部外す
		SetUseVertexShader(-1);
		SetUsePixelShader(-1);
		for (int slot = 1; slot < 16; slot++) SetUseTextureToShader(slot, -1);
		SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 4);
		SetShaderConstantBuffer(-1, DX_SHADERTYPE_PIXEL, 5);
		SetDrawMode(savedDrawMode);
		SetMaxAnisotropy(savedAniso);
	}
} // namespace Mv1LikeUnity
