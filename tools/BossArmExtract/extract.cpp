// ボスのmv1から片腕(clavicle_x以下)だけを切り出して .x → mv1 にする
// 使い方: extract.exe ボス.mv1 出力フォルダ l|r しきい値
// 骨の階層・向きはボスと同じ。頂点の座標は spine_03 の空間(モデルの原点 = spine_03)
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>

static FILE* g_log = nullptr;

struct OutVertex
{
	VECTOR pos, nrm;
	float u, v;
	std::vector<float> w;//arm bone ごと
};

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	fopen_s(&g_log, "extract_log.txt", "a");

	const char* bossPath = __argv[1];
	std::string outDir = __argv[2];
	const char side = __argv[3][0];
	const float threshold = (float)atof(__argv[4]);

	int h = MV1LoadModel(bossPath);
	if (h < 0) { fprintf(g_log, "load fail\n"); return 1; }

	const std::string sfx = side == 'l' ? "_l" : "_r";
	const int spine = MV1SearchFrame(h, "spine_03");
	const int clav = MV1SearchFrame(h, ("clavicle" + sfx).c_str());

	//腕の骨(clavicle以下)を親→子の順に集める
	std::vector<int> bones;
	std::function<void(int)> collect = [&](int f) {
		bones.push_back(f);
		for (int i = 0; i < MV1GetFrameChildNum(h, f); ++i) collect(MV1GetFrameChild(h, f, i));
	};
	collect(clav);
	std::map<int, int> boneIdx;
	for (int i = 0; i < (int)bones.size(); ++i) boneIdx[bones[i]] = i;

	MV1SetupReferenceMesh(h, -1, TRUE);
	MV1_REF_POLYGONLIST r0 = MV1GetReferenceMesh(h, -1, TRUE);
	const int vnum = r0.VertexNum;
	std::vector<MV1_REF_VERTEX> v0(r0.Vertexs, r0.Vertexs + vnum);
	std::vector<MV1_REF_POLYGON> polys(r0.Polygons, r0.Polygons + r0.PolygonNum);

	//S[b][v] = 骨bとその子孫のウェイトの合計(骨bを平行移動したときに頂点がついてくる割合)
	std::vector<std::vector<float>> S(bones.size(), std::vector<float>(vnum, 0.0f));
	for (int bi = 0; bi < (int)bones.size(); ++bi)
	{
		const int b = bones[bi];
		VECTOR p0 = MV1GetFramePosition(h, b);
		MATRIX base = MV1GetFrameBaseLocalMatrix(h, b);
		MV1SetFrameUserLocalMatrix(h, b, MMult(base, MGetTranslate(VGet(3.0f, 4.0f, 5.0f))));
		VECTOR p1 = MV1GetFramePosition(h, b);
		VECTOR d = VSub(p1, p0);
		float dd = VDot(d, d);
		MV1RefreshReferenceMesh(h, -1, TRUE);
		MV1_REF_POLYGONLIST r1 = MV1GetReferenceMesh(h, -1, TRUE);
		for (int v = 0; v < vnum; ++v)
		{
			VECTOR dv = VSub(r1.Vertexs[v].Position, v0[v].Position);
			S[bi][v] = VDot(dv, d) / dd;
		}
		MV1ResetFrameUserLocalMatrix(h, b);
	}
	MV1RefreshReferenceMesh(h, -1, TRUE);

	//骨ごとのウェイト = S(自分) - S(子)の合計
	auto weightOf = [&](int bi, int v) {
		float w = S[bi][v];
		const int b = bones[bi];
		for (int i = 0; i < MV1GetFrameChildNum(h, b); ++i) w -= S[boneIdx[MV1GetFrameChild(h, b, i)]][v];
		return w < 0.0f ? 0.0f : w;
	};

	//メッシュはボスの Pirate_Body フレームに入れる(フレームの並びをボスと同じにするため)
	const int meshFrame = MV1SearchFrame(h, "Pirate_Body");
	MATRIX spineWorld = MV1GetFrameLocalWorldMatrix(h, meshFrame);
	MATRIX invSpine = MInverse(spineWorld);

	std::vector<OutVertex> outV;
	std::vector<int> outF;//3つずつ
	std::vector<int> outMat;
	std::unordered_map<std::string, int> dedup;
	std::map<int, int> matMap;//ボスのマテリアル → .x のマテリアル
	std::vector<int> matList;

	auto addVertex = [&](int v) {
		std::string key((const char*)&v0[v].Position, sizeof(VECTOR) * 2 + sizeof(FLOAT2));
		auto it = dedup.find(key);
		if (it != dedup.end()) return it->second;
		OutVertex o;
		o.pos = VTransform(v0[v].Position, invSpine);
		o.nrm = VNorm(VTransformSR(v0[v].Normal, invSpine));
		o.u = v0[v].TexCoord[0].u; o.v = v0[v].TexCoord[0].v;
		o.w.resize(bones.size());
		float sum = 0.0f;
		for (int bi = 0; bi < (int)bones.size(); ++bi) { o.w[bi] = weightOf(bi, v); sum += o.w[bi]; }
		if (sum < 0.0001f) { o.w[0] = 1.0f; sum = 1.0f; }
		for (auto& w : o.w) w /= sum;
		outV.push_back(o);
		dedup[key] = (int)outV.size() - 1;
		return (int)outV.size() - 1;
	};

	for (auto& p : polys)
	{
		bool keep = true;
		for (int k = 0; k < 3; ++k) if (S[0][p.VIndex[k]] < threshold) keep = false;
		if (!keep) continue;
		if (!matMap.count(p.MaterialIndex)) { matMap[p.MaterialIndex] = (int)matList.size(); matList.push_back(p.MaterialIndex); }
		for (int k = 0; k < 3; ++k) outF.push_back(addVertex(p.VIndex[k]));
		outMat.push_back(matMap[p.MaterialIndex]);
	}
	fprintf(g_log, "side=%c bones=%d faces=%d verts=%d mats=%d\n", side, (int)bones.size(), (int)outMat.size(), (int)outV.size(), (int)matList.size());

	//---- .x を書く ----
	const std::string name = side == 'l' ? "BossArmL" : "BossArmR";
	const std::string xPath = outDir + "/" + name + ".x";
	FILE* f = nullptr; fopen_s(&f, xPath.c_str(), "w");
	fprintf(f, "xof 0303txt 0032\n\n");
	for (int mi = 0; mi < (int)matList.size(); ++mi)
	{
		int tex = MV1GetMaterialDifMapTexture(h, matList[mi]);
		fprintf(f, "Material M%d {\n 1.000000;1.000000;1.000000;1.000000;;\n 10.000000;\n 0.000000;0.000000;0.000000;;\n 0.000000;0.000000;0.000000;;\n", matList[mi]);
		if (tex >= 0)
		{
			std::string tp = MV1GetTextureColorFilePath(h, tex);
			for (auto& c : tp) if (c == '\\') c = '/';
			fprintf(f, " TextureFilename { \"%s\"; }\n", tp.c_str());
		}
		fprintf(f, "}\n");
	}
	auto writeMatrix = [&](const MATRIX& m) {
		for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j)
			fprintf(f, "%.6f%s", m.m[i][j], (i == 3 && j == 3) ? ";;\n" : ",");
	};
	std::function<void()> writeMesh;
	//ボスと同じフレームを同じ順番で書く(アニメはフレーム番号で当てる)
	std::function<void(int)> writeFrame = [&](int b) {
		fprintf(f, "Frame %s {\n FrameTransformMatrix { ", MV1GetFrameName(h, b));
		writeMatrix(MV1GetFrameBaseLocalMatrix(h, b));
		fprintf(f, " }\n");
		if (b == meshFrame) writeMesh();
		for (int i = 0; i < MV1GetFrameChildNum(h, b); ++i) writeFrame(MV1GetFrameChild(h, b, i));
		fprintf(f, "}\n");
	};

	writeMesh = [&]() {
	fprintf(f, " Mesh {\n  %d;\n", (int)outV.size());
	for (size_t i = 0; i < outV.size(); ++i) fprintf(f, "  %.5f;%.5f;%.5f;%s\n", outV[i].pos.x, outV[i].pos.y, outV[i].pos.z, i + 1 == outV.size() ? ";" : ",");
	const int fnum = (int)outMat.size();
	fprintf(f, "  %d;\n", fnum);
	for (int i = 0; i < fnum; ++i) fprintf(f, "  3;%d,%d,%d;%s\n", outF[i * 3], outF[i * 3 + 1], outF[i * 3 + 2], i + 1 == fnum ? ";" : ",");

	fprintf(f, "  MeshNormals {\n   %d;\n", (int)outV.size());
	for (size_t i = 0; i < outV.size(); ++i) fprintf(f, "   %.5f;%.5f;%.5f;%s\n", outV[i].nrm.x, outV[i].nrm.y, outV[i].nrm.z, i + 1 == outV.size() ? ";" : ",");
	fprintf(f, "   %d;\n", fnum);
	for (int i = 0; i < fnum; ++i) fprintf(f, "   3;%d,%d,%d;%s\n", outF[i * 3], outF[i * 3 + 1], outF[i * 3 + 2], i + 1 == fnum ? ";" : ",");
	fprintf(f, "  }\n");

	fprintf(f, "  MeshTextureCoords {\n   %d;\n", (int)outV.size());
	for (size_t i = 0; i < outV.size(); ++i) fprintf(f, "   %.6f;%.6f;%s\n", outV[i].u, outV[i].v, i + 1 == outV.size() ? ";" : ",");
	fprintf(f, "  }\n");

	fprintf(f, "  MeshMaterialList {\n   %d;\n   %d;\n", (int)matList.size(), fnum);
	for (int i = 0; i < fnum; ++i) fprintf(f, "   %d%s\n", outMat[i], i + 1 == fnum ? ";" : ",");
	for (int mi = 0; mi < (int)matList.size(); ++mi) fprintf(f, "   {M%d}\n", matList[mi]);
	fprintf(f, "  }\n");

	//スキン
	std::vector<int> usedBones;
	for (int bi = 0; bi < (int)bones.size(); ++bi)
	{
		for (auto& o : outV) if (o.w[bi] > 0.0001f) { usedBones.push_back(bi); break; }
	}
	fprintf(f, "  XSkinMeshHeader {\n   4;\n   12;\n   %d;\n  }\n", (int)usedBones.size());
	for (int bi : usedBones)
	{
		std::vector<int> idx; std::vector<float> ws;
		for (size_t i = 0; i < outV.size(); ++i) if (outV[i].w[bi] > 0.0001f) { idx.push_back((int)i); ws.push_back(outV[i].w[bi]); }
		fprintf(f, "  SkinWeights {\n   \"%s\";\n   %d;\n", MV1GetFrameName(h, bones[bi]), (int)idx.size());
		for (size_t i = 0; i < idx.size(); ++i) fprintf(f, "   %d%s\n", idx[i], i + 1 == idx.size() ? ";" : ",");
		for (size_t i = 0; i < ws.size(); ++i) fprintf(f, "   %.6f%s\n", ws[i], i + 1 == ws.size() ? ";" : ",");
		//メッシュの空間(spine_03) → 骨の空間
		MATRIX boneWorld = MV1GetFrameLocalWorldMatrix(h, bones[bi]);
		fprintf(f, "   ");
		writeMatrix(MMult(spineWorld, MInverse(boneWorld)));
		fprintf(f, "  }\n");
	}
	fprintf(f, " }\n");
	};

	for (int i = 0; i < MV1GetFrameNum(h); ++i) if (MV1GetFrameParent(h, i) == -2) writeFrame(i);
	fclose(f);

	//---- 読み直して、マテリアルの設定をボスと合わせて mv1 に保存 ----
	int hx = MV1LoadModel(xPath.c_str());
	fprintf(g_log, "reload %s handle=%d frames=%d meshes=%d mats=%d\n", xPath.c_str(), hx, hx >= 0 ? MV1GetFrameNum(hx) : -1, hx >= 0 ? MV1GetMeshNum(hx) : -1, hx >= 0 ? MV1GetMaterialNum(hx) : -1);
	if (hx >= 0)
	{
		for (int mi = 0; mi < MV1GetMaterialNum(hx); ++mi)
		{
			const char* mn = MV1GetMaterialName(hx, mi);
			int bossMat = (mn && mn[0] == 'M') ? atoi(mn + 1) : -1;
			if (bossMat < 0) continue;
			MV1SetMaterialDrawAlphaTest(hx, mi, MV1GetMaterialDrawAlphaTestEnable(h, bossMat), MV1GetMaterialDrawAlphaTestMode(h, bossMat), MV1GetMaterialDrawAlphaTestParam(h, bossMat));
			MV1SetMaterialDifColor(hx, mi, MV1GetMaterialDifColor(h, bossMat));
			fprintf(g_log, "  mat[%d] %s tex=%d\n", mi, mn, MV1GetMaterialDifMapTexture(hx, mi));
		}
		const std::string mvPath = outDir + "/" + name + ".mv1";
		int ret = MV1SaveModelToMV1File(hx, mvPath.c_str());
		fprintf(g_log, "save %s ret=%d\n", mvPath.c_str(), ret);
	}
	fclose(g_log);
	DxLib_End();
	return 0;
}
