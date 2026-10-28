// Kingmech(ToiletMech.mv1)の右腕のガトリング(polySurface789)だけを切り出して、骨なしの mv1 にする(部位破壊で落とす用)
// 使い方: extract.exe ToiletMech.mv1 出力フォルダ   ※出力フォルダに KingmechGatling.x / .mv1・extract_log.txt
// 頂点は lowerarm_r の空間(回転だけ)で、原点はガトリングの真ん中。肩へのケーブルは外す
#include "DxLib.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace
{
	const char* kMeshFrame = "polySurface789";//ガトリングのメッシュのフレーム
	const char* kBone = "lowerarm_r";//ガトリングが付いている骨
	const char* kName = "KingmechGatling";
	constexpr float kFollowRate = 0.99f;//骨についてくる割合がこれ未満の頂点を含むかたまり(ケーブル)は外す
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (__argc < 3 || DxLib_Init() == -1) return 1;
	const std::string outDir = __argv[2];
	FILE* log = nullptr;
	fopen_s(&log, (outDir + "/extract_log.txt").c_str(), "w");

	int h = MV1LoadModel(__argv[1]);
	const int mf = MV1SearchFrame(h, kMeshFrame);
	const int bone = MV1SearchFrame(h, kBone);
	fprintf(log, "load %d meshFrame %d bone %d\n", h, mf, bone);
	if (h < 0 || mf < 0 || bone < 0) { fclose(log); DxLib_End(); return 2; }

	MV1SetupReferenceMesh(h, mf, TRUE);
	MV1_REF_POLYGONLIST r0 = MV1GetReferenceMesh(h, mf, TRUE);
	const int vn = r0.VertexNum;
	std::vector<MV1_REF_VERTEX> v0(r0.Vertexs, r0.Vertexs + vn);
	std::vector<MV1_REF_POLYGON> polys(r0.Polygons, r0.Polygons + r0.PolygonNum);

	//骨を平行移動して、頂点がどれだけついてくるか
	std::vector<float> follow(vn);
	{
		VECTOR a = MV1GetFramePosition(h, bone);
		MV1SetFrameUserLocalMatrix(h, bone, MMult(MV1GetFrameBaseLocalMatrix(h, bone), MGetTranslate(VGet(3, 4, 5))));
		VECTOR d = VSub(MV1GetFramePosition(h, bone), a);
		MV1RefreshReferenceMesh(h, mf, TRUE);
		MV1_REF_POLYGONLIST r1 = MV1GetReferenceMesh(h, mf, TRUE);
		for (int v = 0; v < vn; ++v) follow[v] = VDot(VSub(r1.Vertexs[v].Position, v0[v].Position), d) / VDot(d, d);
		MV1ResetFrameUserLocalMatrix(h, bone);
	}

	//同じ位置の頂点・三角形でつながったかたまりに分ける
	std::vector<int> parent(vn);
	for (int v = 0; v < vn; ++v) parent[v] = v;
	auto root = [&](int v) { while (parent[v] != v) v = parent[v] = parent[parent[v]]; return v; };
	auto unite = [&](int a, int b) { parent[root(a)] = root(b); };
	{
		std::unordered_map<std::string, int> samePos;
		for (int v = 0; v < vn; ++v)
		{
			std::string key((const char*)&v0[v].Position, sizeof(VECTOR));
			auto it = samePos.find(key);
			if (it == samePos.end()) samePos[key] = v;
			else unite(v, it->second);
		}
		for (auto& p : polys) { unite(p.VIndex[0], p.VIndex[1]); unite(p.VIndex[1], p.VIndex[2]); }
	}
	std::map<int, bool> dropGroup;
	for (int v = 0; v < vn; ++v) if (follow[v] < kFollowRate) dropGroup[root(v)] = true;

	//骨の空間(回転だけ)
	MATRIX boneWorld = MV1GetFrameLocalWorldMatrix(h, bone);
	VECTOR ax = VNorm(VGet(boneWorld.m[0][0], boneWorld.m[0][1], boneWorld.m[0][2]));
	VECTOR ay = VNorm(VGet(boneWorld.m[1][0], boneWorld.m[1][1], boneWorld.m[1][2]));
	VECTOR az = VNorm(VGet(boneWorld.m[2][0], boneWorld.m[2][1], boneWorld.m[2][2]));
	fprintf(log, "bone axis length %.4f %.4f %.4f\n",
		VSize(VGet(boneWorld.m[0][0], boneWorld.m[0][1], boneWorld.m[0][2])),
		VSize(VGet(boneWorld.m[1][0], boneWorld.m[1][1], boneWorld.m[1][2])),
		VSize(VGet(boneWorld.m[2][0], boneWorld.m[2][1], boneWorld.m[2][2])));
	MATRIX boneRot = MGetIdent();
	boneRot.m[0][0] = ax.x; boneRot.m[0][1] = ax.y; boneRot.m[0][2] = ax.z;
	boneRot.m[1][0] = ay.x; boneRot.m[1][1] = ay.y; boneRot.m[1][2] = ay.z;
	boneRot.m[2][0] = az.x; boneRot.m[2][1] = az.y; boneRot.m[2][2] = az.z;
	boneRot.m[3][0] = boneWorld.m[3][0]; boneRot.m[3][1] = boneWorld.m[3][1]; boneRot.m[3][2] = boneWorld.m[3][2];
	MATRIX invBone = MInverse(boneRot);

	//残す三角形と、その範囲(骨の空間)
	std::vector<int> keepPolys;
	VECTOR mn = VGet(1e9f, 1e9f, 1e9f), mx = VGet(-1e9f, -1e9f, -1e9f);
	for (int i = 0; i < (int)polys.size(); ++i)
	{
		if (dropGroup.count(root(polys[i].VIndex[0]))) continue;
		keepPolys.push_back(i);
		for (int k = 0; k < 3; ++k)
		{
			VECTOR p = VTransform(v0[polys[i].VIndex[k]].Position, invBone);
			mn = VGet((std::fminf)(mn.x, p.x), (std::fminf)(mn.y, p.y), (std::fminf)(mn.z, p.z));
			mx = VGet((std::fmaxf)(mx.x, p.x), (std::fmaxf)(mx.y, p.y), (std::fmaxf)(mx.z, p.z));
		}
	}
	const VECTOR center = VScale(VAdd(mn, mx), 0.5f);
	fprintf(log, "polys %d -> %d (drop groups %d)\n", (int)polys.size(), (int)keepPolys.size(), (int)dropGroup.size());
	fprintf(log, "size(%.1f %.1f %.1f)\n", mx.x - mn.x, mx.y - mn.y, mx.z - mn.z);
	fprintf(log, "center in %s space (offset) = VGet(%.3ff, %.3ff, %.3ff)\n", kBone, center.x, center.y, center.z);

	//頂点をまとめる
	struct OutVertex { VECTOR pos, nrm; float u, v; };
	std::vector<OutVertex> outV;
	std::vector<int> outF, outMat, matList;
	std::map<int, int> matMap;
	std::unordered_map<std::string, int> dedup;
	auto addVertex = [&](int v)
	{
		std::string key((const char*)&v0[v].Position, sizeof(VECTOR) * 2 + sizeof(FLOAT2));
		auto it = dedup.find(key);
		if (it != dedup.end()) return it->second;
		OutVertex o;
		o.pos = VSub(VTransform(v0[v].Position, invBone), center);
		o.nrm = VNorm(VTransformSR(v0[v].Normal, invBone));
		o.u = v0[v].TexCoord[0].u; o.v = v0[v].TexCoord[0].v;
		outV.push_back(o);
		return dedup[key] = (int)outV.size() - 1;
	};
	for (int i : keepPolys)
	{
		auto& p = polys[i];
		if (!matMap.count(p.MaterialIndex)) { matMap[p.MaterialIndex] = (int)matList.size(); matList.push_back(p.MaterialIndex); }
		for (int k = 0; k < 3; ++k) outF.push_back(addVertex(p.VIndex[k]));
		outMat.push_back(matMap[p.MaterialIndex]);
	}

	//---- .x を書く ----
	const std::string xPath = outDir + "/" + kName + ".x";
	FILE* f = nullptr;
	fopen_s(&f, xPath.c_str(), "w");
	fprintf(f, "xof 0303txt 0032\n\n");
	for (int m : matList)
	{
		fprintf(f, "Material M%d {\n 1.000000;1.000000;1.000000;1.000000;;\n 10.000000;\n 0.000000;0.000000;0.000000;;\n 0.000000;0.000000;0.000000;;\n", m);
		int tex = MV1GetMaterialDifMapTexture(h, m);
		if (tex >= 0)
		{
			std::string tp = MV1GetTextureColorFilePath(h, tex);
			for (auto& c : tp) if (c == '\\') c = '/';
			fprintf(f, " TextureFilename { \"%s\"; }\n", tp.c_str());
		}
		fprintf(f, "}\n");
	}
	const int fnum = (int)outMat.size();
	const int vnum = (int)outV.size();
	auto sep = [](int i, int n) { return i + 1 == n ? ";" : ","; };
	fprintf(f, "Frame %s {\n FrameTransformMatrix { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1;; }\n Mesh {\n  %d;\n", kName, vnum);
	for (int i = 0; i < vnum; ++i) fprintf(f, "  %.5f;%.5f;%.5f;%s\n", outV[i].pos.x, outV[i].pos.y, outV[i].pos.z, sep(i, vnum));
	fprintf(f, "  %d;\n", fnum);
	for (int i = 0; i < fnum; ++i) fprintf(f, "  3;%d,%d,%d;%s\n", outF[i * 3], outF[i * 3 + 1], outF[i * 3 + 2], sep(i, fnum));
	fprintf(f, "  MeshNormals {\n   %d;\n", vnum);
	for (int i = 0; i < vnum; ++i) fprintf(f, "   %.5f;%.5f;%.5f;%s\n", outV[i].nrm.x, outV[i].nrm.y, outV[i].nrm.z, sep(i, vnum));
	fprintf(f, "   %d;\n", fnum);
	for (int i = 0; i < fnum; ++i) fprintf(f, "   3;%d,%d,%d;%s\n", outF[i * 3], outF[i * 3 + 1], outF[i * 3 + 2], sep(i, fnum));
	fprintf(f, "  }\n  MeshTextureCoords {\n   %d;\n", vnum);
	for (int i = 0; i < vnum; ++i) fprintf(f, "   %.6f;%.6f;%s\n", outV[i].u, outV[i].v, sep(i, vnum));
	fprintf(f, "  }\n  MeshMaterialList {\n   %d;\n   %d;\n", (int)matList.size(), fnum);
	for (int i = 0; i < fnum; ++i) fprintf(f, "   %d%s\n", outMat[i], sep(i, fnum));
	for (int m : matList) fprintf(f, "   {M%d}\n", m);
	fprintf(f, "  }\n }\n}\n");
	fclose(f);

	//---- 読み直して、マテリアルの色をボスと合わせて mv1 に保存 ----
	int hx = MV1LoadModel(xPath.c_str());
	fprintf(log, "reload %d verts %d faces %d mats %d\n", hx, vnum, fnum, (int)matList.size());
	if (hx >= 0)
	{
		for (int mi = 0; mi < MV1GetMaterialNum(hx); ++mi)
		{
			const char* name = MV1GetMaterialName(hx, mi);
			int src = (name && name[0] == 'M') ? atoi(name + 1) : -1;
			if (src < 0) continue;
			MV1SetMaterialDifColor(hx, mi, MV1GetMaterialDifColor(h, src));
			MV1SetMaterialAmbColor(hx, mi, MV1GetMaterialAmbColor(h, src));
			MV1SetMaterialSpcColor(hx, mi, MV1GetMaterialSpcColor(h, src));
			MV1SetMaterialEmiColor(hx, mi, MV1GetMaterialEmiColor(h, src));
			MV1SetMaterialSpcPower(hx, mi, MV1GetMaterialSpcPower(h, src));
			MV1SetMaterialDrawAlphaTest(hx, mi, MV1GetMaterialDrawAlphaTestEnable(h, src), MV1GetMaterialDrawAlphaTestMode(h, src), MV1GetMaterialDrawAlphaTestParam(h, src));
			fprintf(log, "  mat[%d] %s tex %d\n", mi, name, MV1GetMaterialDifMapTexture(hx, mi));
		}
		int ret = MV1SaveModelToMV1File(hx, (outDir + "/" + kName + ".mv1").c_str());
		fprintf(log, "save ret %d\n", ret);
	}
	fclose(log);
	DxLib_End();
	return 0;
}
