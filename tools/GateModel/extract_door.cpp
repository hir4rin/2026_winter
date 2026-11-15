// 列車ステージの4両目の前の扉(corridor_door)を左右に分けて切り出し、扉(ゲート)のモデルにする
// 使い方: extract_door.exe TrainStage.mv1 出力フォルダ
//   出力: CorridorDoorL.mv1 / CorridorDoorR.mv1(原点 = 扉の外側の端・床・扉の厚みの中央)、GateBox.mv1(当たり判定用の1m角の箱、原点が中心)
//   テクスチャは 出力フォルダ/tex/corridor_door.jpg を使う(先にコピーしておく)
#include "DxLib.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>

namespace
{
	constexpr float kDoorZMin = 10650.0f;//4両目の前の扉だけを取る範囲
	constexpr float kDoorZMax = 10800.0f;
	FILE* g_log = nullptr;
}

struct OutMesh
{
	std::vector<VECTOR> pos, nrm;
	std::vector<FLOAT2> uv;
	std::vector<int> faces;
};

static void WriteX(const std::string& path, const OutMesh& m, const char* tex, const char* frameName)
{
	FILE* f = nullptr; fopen_s(&f, path.c_str(), "w");
	fprintf(f, "xof 0303txt 0032\n");
	fprintf(f, "Material M0 {\n 1.0;1.0;1.0;1.0;;\n 10.0;\n 0.0;0.0;0.0;;\n 0.0;0.0;0.0;;\n");
	if (tex) fprintf(f, " TextureFilename { \"%s\"; }\n", tex);
	fprintf(f, "}\n");
	fprintf(f, "Frame %s {\n FrameTransformMatrix { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1;; }\n Mesh {\n  %d;\n", frameName, (int)m.pos.size());
	for (size_t i = 0; i < m.pos.size(); ++i) fprintf(f, "  %.3f;%.3f;%.3f;%s\n", m.pos[i].x, m.pos[i].y, m.pos[i].z, i + 1 == m.pos.size() ? ";" : ",");
	const int fn = (int)m.faces.size() / 3;
	fprintf(f, "  %d;\n", fn);
	for (int i = 0; i < fn; ++i) fprintf(f, "  3;%d,%d,%d;%s\n", m.faces[i * 3], m.faces[i * 3 + 1], m.faces[i * 3 + 2], i + 1 == fn ? ";" : ",");
	fprintf(f, "  MeshNormals {\n   %d;\n", (int)m.nrm.size());
	for (size_t i = 0; i < m.nrm.size(); ++i) fprintf(f, "   %.4f;%.4f;%.4f;%s\n", m.nrm[i].x, m.nrm[i].y, m.nrm[i].z, i + 1 == m.nrm.size() ? ";" : ",");
	fprintf(f, "   %d;\n", fn);
	for (int i = 0; i < fn; ++i) fprintf(f, "   3;%d,%d,%d;%s\n", m.faces[i * 3], m.faces[i * 3 + 1], m.faces[i * 3 + 2], i + 1 == fn ? ";" : ",");
	fprintf(f, "  }\n  MeshTextureCoords {\n   %d;\n", (int)m.uv.size());
	for (size_t i = 0; i < m.uv.size(); ++i) fprintf(f, "   %.5f;%.5f;%s\n", m.uv[i].u, m.uv[i].v, i + 1 == m.uv.size() ? ";" : ",");
	fprintf(f, "  }\n  MeshMaterialList {\n   1;\n   %d;\n", fn);
	for (int i = 0; i < fn; ++i) fprintf(f, "   0%s\n", i + 1 == fn ? ";" : ",");
	fprintf(f, "   {M0}\n  }\n }\n}\n");
	fclose(f);
}

//.x を読み直して、マテリアルを元のモデルと同じにして mv1 で保存する
static void SaveMv1(const std::string& xPath, const std::string& mvPath, int src, int srcMat)
{
	int h = MV1LoadModel(xPath.c_str());
	if (h < 0) { fprintf(g_log, "reload fail %s\n", xPath.c_str()); return; }
	for (int i = 0; i < MV1GetMaterialNum(h); ++i)
	{
		if (src >= 0)
		{
			MV1SetMaterialDifColor(h, i, MV1GetMaterialDifColor(src, srcMat));
			MV1SetMaterialAmbColor(h, i, MV1GetMaterialAmbColor(src, srcMat));
			MV1SetMaterialSpcColor(h, i, MV1GetMaterialSpcColor(src, srcMat));
			MV1SetMaterialEmiColor(h, i, MV1GetMaterialEmiColor(src, srcMat));
			MV1SetMaterialSpcPower(h, i, MV1GetMaterialSpcPower(src, srcMat));
		}
	}
	MV1SetupReferenceMesh(h, -1, TRUE);
	MV1_REF_POLYGONLIST r = MV1GetReferenceMesh(h, -1, TRUE);
	int ret = MV1SaveModelToMV1File(h, mvPath.c_str());
	fprintf(g_log, "save %s ret=%d polys=%d tex=%d min(%.0f %.0f %.0f) max(%.0f %.0f %.0f)\n", mvPath.c_str(), ret, r.PolygonNum, MV1GetMaterialDifMapTexture(h, 0),
		r.MinPosition.x, r.MinPosition.y, r.MinPosition.z, r.MaxPosition.x, r.MaxPosition.y, r.MaxPosition.z);
	MV1DeleteModel(h);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	fopen_s(&g_log, "extract_door_log.txt", "w");
	const std::string outDir = __argv[2];
	int h = MV1LoadModel(__argv[1]);
	MV1SetupReferenceMesh(h, -1, TRUE);
	MV1_REF_POLYGONLIST r = MV1GetReferenceMesh(h, -1, TRUE);

	//扉の面を集めて、左右(面の中心のx)に分ける
	std::vector<int> sides[2];
	int doorMat = -1;
	float minX = 1e9f, maxX = -1e9f, minZ = 1e9f, maxZ = -1e9f, minY = 1e9f;
	for (int i = 0; i < r.PolygonNum; ++i)
	{
		auto& p = r.Polygons[i];
		int t = MV1GetMaterialDifMapTexture(h, p.MaterialIndex);
		if (t < 0 || !strstr(MV1GetTextureColorFilePath(h, t), "corridor_door")) continue;
		VECTOR c = VScale(VAdd(VAdd(r.Vertexs[p.VIndex[0]].Position, r.Vertexs[p.VIndex[1]].Position), r.Vertexs[p.VIndex[2]].Position), 1.0f / 3.0f);
		if (c.z < kDoorZMin || c.z > kDoorZMax) continue;
		doorMat = p.MaterialIndex;
		sides[c.x < 0.0f ? 0 : 1].push_back(i);
		for (int k = 0; k < 3; ++k)
		{
			VECTOR v = r.Vertexs[p.VIndex[k]].Position;
			minX = (std::min)(minX, v.x); maxX = (std::max)(maxX, v.x);
			minZ = (std::min)(minZ, v.z); maxZ = (std::max)(maxZ, v.z); minY = (std::min)(minY, v.y);
		}
	}
	fprintf(g_log, "door mat=%d L=%d R=%d x %.1f..%.1f z %.1f..%.1f minY %.1f\n", doorMat, (int)sides[0].size(), (int)sides[1].size(), minX, maxX, minZ, maxZ, minY);
	const float centerZ = (minZ + maxZ) * 0.5f;

	const char* names[2] = { "CorridorDoorL", "CorridorDoorR" };
	for (int s = 0; s < 2; ++s)
	{
		//原点 = 扉の外側の端
		const VECTOR pivot = VGet(s == 0 ? minX : maxX, minY, centerZ);
		OutMesh m;
		std::map<int, int> remap;
		for (int pi : sides[s])
		{
			auto& p = r.Polygons[pi];
			for (int k = 0; k < 3; ++k)
			{
				int vi = p.VIndex[k];
				auto it = remap.find(vi);
				if (it == remap.end())
				{
					auto& v = r.Vertexs[vi];
					m.pos.push_back(VSub(v.Position, pivot));
					m.nrm.push_back(v.Normal);
					m.uv.push_back(FLOAT2{ v.TexCoord[0].u, v.TexCoord[0].v });
					it = remap.emplace(vi, (int)m.pos.size() - 1).first;
				}
				m.faces.push_back(it->second);
			}
		}
		const std::string x = outDir + "/" + names[s] + ".x";
		WriteX(x, m, "tex/corridor_door.jpg", names[s]);
		SaveMv1(x, outDir + "/" + names[s] + ".mv1", h, doorMat);
		fprintf(g_log, "%s pivot (%.1f %.1f %.1f)\n", names[s], pivot.x, pivot.y, pivot.z);
		remove(x.c_str());
	}

	//当たり判定の箱(1m角、原点が中心、面は外向き)
	{
		OutMesh m;
		const float s = 50.0f;
		const VECTOR n[6] = { VGet(1,0,0), VGet(-1,0,0), VGet(0,1,0), VGet(0,-1,0), VGet(0,0,1), VGet(0,0,-1) };
		for (int fi = 0; fi < 6; ++fi)
		{
			VECTOR a = n[fi];
			VECTOR u = fabsf(a.y) > 0.5f ? VGet(1, 0, 0) : VGet(0, 1, 0);
			VECTOR v = VCross(a, u);
			int base = (int)m.pos.size();
			const float c[4][2] = { {-1,-1}, {1,-1}, {1,1}, {-1,1} };
			for (auto& cc : c)
			{
				m.pos.push_back(VScale(VAdd(a, VAdd(VScale(u, cc[0]), VScale(v, cc[1]))), s));
				m.nrm.push_back(a);
				m.uv.push_back(FLOAT2{ 0, 0 });
			}
			//DxLib(左手系)で外から見て時計回り
			int q[6] = { 0, 1, 2, 0, 2, 3 };
			VECTOR e1 = VSub(m.pos[base + 1], m.pos[base]), e2 = VSub(m.pos[base + 2], m.pos[base]);
			bool flip = VDot(VCross(e1, e2), a) < 0.0f;
			for (int k = 0; k < 6; k += 3)
			{
				m.faces.push_back(base + q[k]);
				m.faces.push_back(base + (flip ? q[k + 2] : q[k + 1]));
				m.faces.push_back(base + (flip ? q[k + 1] : q[k + 2]));
			}
		}
		const std::string x = outDir + "/GateBox.x";
		WriteX(x, m, nullptr, "GateBox");
		SaveMv1(x, outDir + "/GateBox.mv1", -1, 0);
		remove(x.c_str());
	}
	fclose(g_log);
	DxLib_End();
	return 0;
}
