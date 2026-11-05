// .x の中の髪のメッシュ(格子状の板の集まり)を、行・列を間引いて減らす
// hairlod.exe 入力.x 出力.x フレーム名 長さの間引き(1,2,3..) 幅の間引き(1,2..) ログ
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <numeric>
#include <queue>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

struct Vert
{
	float p[3], n[3], uv[2];
	std::vector<std::pair<int, float>> w;
};

static int ParseCount(const std::string& s) { return atoi(s.c_str()); }
static std::string Trim(const std::string& s)
{
	size_t a = s.find_first_not_of(" \t\r");
	size_t b = s.find_last_not_of(" \t\r");
	return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
static std::vector<int> KeepList(int n, int step)
{
	std::vector<int> k;
	for (int i = 0; i < n; i += step) k.push_back(i);
	k.push_back(n);
	//最後の区間が短すぎるときは1つ前を外す
	if (k.size() >= 3 && step > 1 && (k[k.size() - 1] - k[k.size() - 2]) * 2 < step) k.erase(k.end() - 2);
	return k;
}

int main(int argc, char** argv)
{
	if (argc < 7) return 1;
	const std::string frameName = argv[3];
	const int lenStep = atoi(argv[4]);
	const int widStep = atoi(argv[5]);
	FILE* log = nullptr;
	fopen_s(&log, argv[6], "w");

	std::vector<std::string> l;
	{
		std::ifstream in(argv[1]);
		std::string s;
		while (std::getline(in, s)) l.push_back(s);
	}
	int frameLine = -1;
	for (int i = 0; i < (int)l.size(); ++i) if (Trim(l[i]) == "Frame " + frameName + " {") { frameLine = i; break; }
	if (frameLine < 0) { fprintf(log, "frame not found\n"); return 2; }
	int i = frameLine;
	while (Trim(l[i]) != "Mesh {") ++i;
	const int meshLine = i;
	const int vnum = ParseCount(Trim(l[++i]));
	++i;
	std::vector<Vert> v(vnum);
	for (int k = 0; k < vnum; ++k, ++i) sscanf_s(Trim(l[i]).c_str(), "%f;%f;%f;", &v[k].p[0], &v[k].p[1], &v[k].p[2]);
	const int fnum = ParseCount(Trim(l[i++]));
	std::vector<std::array<int, 3>> face(fnum);
	for (int k = 0; k < fnum; ++k, ++i) { int c; sscanf_s(Trim(l[i]).c_str(), "%d;%d,%d,%d;", &c, &face[k][0], &face[k][1], &face[k][2]); }
	while (Trim(l[i]) != "MeshNormals {") ++i;
	i += 2;
	for (int k = 0; k < vnum; ++k, ++i) sscanf_s(Trim(l[i]).c_str(), "%f;%f;%f;", &v[k].n[0], &v[k].n[1], &v[k].n[2]);
	while (Trim(l[i]) != "MeshTextureCoords {") ++i;
	i += 2;
	for (int k = 0; k < vnum; ++k, ++i) sscanf_s(Trim(l[i]).c_str(), "%f;%f;", &v[k].uv[0], &v[k].uv[1]);
	while (Trim(l[i]) != "MeshMaterialList {") ++i;
	const int matLine = i;
	while (Trim(l[i]).rfind("{MAT", 0) != 0) ++i;
	const std::string matRef = l[i];
	while (Trim(l[i]) != "XSkinMeshHeader {") ++i;
	const int skinHeader = i;
	const int maxPerVert = ParseCount(Trim(l[i + 1]));
	const int maxPerFace = ParseCount(Trim(l[i + 2]));
	i += 5;
	struct Bone { std::string name; std::vector<std::string> mat; };
	std::vector<Bone> bones;
	while (Trim(l[i]) == "SkinWeights {")
	{
		Bone b;
		b.name = Trim(l[i + 1]);
		const int n = ParseCount(Trim(l[i + 2]));
		i += 3;
		const int boneId = (int)bones.size();
		std::vector<int> idx(n);
		for (int k = 0; k < n; ++k, ++i) idx[k] = atoi(Trim(l[i]).c_str());
		for (int k = 0; k < n; ++k, ++i) v[idx[k]].w.push_back({ boneId, (float)atof(Trim(l[i]).c_str()) });
		for (int k = 0; k < 4; ++k, ++i) b.mat.push_back(l[i]);
		++i;//}
		bones.push_back(b);
	}
	const int meshEnd = i;//Mesh を閉じる }
	fprintf(log, "verts=%d faces=%d bones=%d meshLine=%d end=[%s]\n", vnum, fnum, (int)bones.size(), meshLine, Trim(l[meshEnd]).c_str());

	//位置・UV・法線が同じ頂点をまとめる(背中合わせの板は法線が違うので別になる)
	std::map<std::array<int, 8>, int> weldMap;
	std::vector<int> weld(vnum), rep;
	for (int k = 0; k < vnum; ++k)
	{
		std::array<int, 8> key = { (int)std::lround(v[k].p[0] * 2000), (int)std::lround(v[k].p[1] * 2000), (int)std::lround(v[k].p[2] * 2000),
			(int)std::lround(v[k].uv[0] * 20000), (int)std::lround(v[k].uv[1] * 20000),
			(int)std::lround(v[k].n[0] * 200), (int)std::lround(v[k].n[1] * 200), (int)std::lround(v[k].n[2] * 200) };
		auto it = weldMap.find(key);
		if (it == weldMap.end()) { it = weldMap.emplace(key, (int)rep.size()).first; rep.push_back(k); }
		weld[k] = it->second;
	}
	const int wnum = (int)rep.size();
	fprintf(log, "welded=%d\n", wnum);
	std::vector<std::array<int, 3>> tri(fnum);
	for (int k = 0; k < fnum; ++k) for (int c = 0; c < 3; ++c) tri[k][c] = weld[face[k][c]];

	//つながり
	std::vector<int> par(wnum);
	std::iota(par.begin(), par.end(), 0);
	auto find = [&](int x) { while (par[x] != x) { par[x] = par[par[x]]; x = par[x]; } return x; };
	for (auto& t : tri) { par[find(t[0])] = find(t[1]); par[find(t[1])] = find(t[2]); }
	std::map<int, std::vector<int>> cards;
	for (int k = 0; k < fnum; ++k) cards[find(tri[k][0])].push_back(k);

	auto uvOf = [&](int w) { return v[rep[w]].uv; };
	auto edgeKey = [](int a, int b) { return (long long)(std::min)(a, b) << 32 | (unsigned)(std::max)(a, b); };

	std::vector<std::array<int, 3>> outTri;
	std::map<std::pair<int, int>, int> dimHist;
	int irregular = 0, irregularTris = 0;
	for (auto& [id, list] : cards)
	{
		bool ok = true;
		//三角形の対角線(UVで斜めの辺)
		std::unordered_map<long long, std::vector<int>> edgeTris;
		std::unordered_map<int, int> diagOf;//tri → その対角線の向かいの頂点の位置(0..2)
		for (int t : list)
		{
			float best = -1; int bestC = -1;
			for (int c = 0; c < 3; ++c)
			{
				const int a = tri[t][c], b = tri[t][(c + 1) % 3];
				const float du = uvOf(a)[0] - uvOf(b)[0], dv = uvOf(a)[1] - uvOf(b)[1];
				const float len = std::fabs(du) * std::fabs(dv);
				if (len > best) { best = len; bestC = c; }
				edgeTris[edgeKey(a, b)].push_back(t);
			}
			diagOf[t] = bestC;
		}
		//対角線を共有する2つの三角形を四角にする
		struct Quad { std::array<int, 4> c; };
		std::vector<Quad> quads;
		std::unordered_map<int, int> triQuad;
		for (int t : list)
		{
			if (triQuad.count(t)) continue;
			const int c = diagOf[t];
			const int a = tri[t][c], b = tri[t][(c + 1) % 3], x = tri[t][(c + 2) % 3];
			int mate = -1;
			for (int o : edgeTris[edgeKey(a, b)])
			{
				if (o == t || triQuad.count(o)) continue;
				const int oc = diagOf[o];
				if (edgeKey(tri[o][oc], tri[o][(oc + 1) % 3]) == edgeKey(a, b)) mate = o;
			}
			if (mate < 0) { ok = false; break; }
			int y = -1;
			for (int q : tri[mate]) if (q != a && q != b) y = q;
			triQuad[t] = (int)quads.size();
			triQuad[mate] = (int)quads.size();
			quads.push_back({ { a, y, b, x } });
		}
		if (!ok) { ++irregular; irregularTris += (int)list.size(); for (int t : list) outTri.push_back(tri[t]); continue; }

		//四角の隣をたどって格子の座標を付ける
		std::unordered_map<long long, std::vector<int>> edgeQuads;
		for (int q = 0; q < (int)quads.size(); ++q) for (int c = 0; c < 4; ++c) edgeQuads[edgeKey(quads[q].c[c], quads[q].c[(c + 1) % 4])].push_back(q);
		std::unordered_map<int, std::pair<int, int>> gridOf;
		std::vector<char> visited(quads.size(), 0);
		std::queue<int> open;
		const auto& q0 = quads[0].c;
		gridOf[q0[0]] = { 0, 0 }; gridOf[q0[1]] = { 1, 0 }; gridOf[q0[2]] = { 1, 1 }; gridOf[q0[3]] = { 0, 1 };
		visited[0] = 1;
		open.push(0);
		while (!open.empty() && ok)
		{
			const int q = open.front(); open.pop();
			const auto c = quads[q].c;
			for (int e = 0; e < 4 && ok; ++e)
			{
				const int a = c[e], b = c[(e + 1) % 4];
				auto& nb = edgeQuads[edgeKey(a, b)];
				if (nb.size() > 2) { ok = false; break; }
				for (int o : nb)
				{
					if (o == q || visited[o]) continue;
					//この四角の残りの辺で向きを決める
					const auto ga = gridOf[a], gb = gridOf[b], gd = gridOf[c[(e + 3) % 4]];
					const int sx = ga.first - gd.first, sy = ga.second - gd.second;//a から外へ向かう向き
					const auto& oc = quads[o].c;
					int ia = -1;
					for (int k = 0; k < 4; ++k) if (oc[k] == a) ia = k;
					if (ia < 0) { ok = false; break; }
					const int nextA = oc[(ia + 1) % 4] == b ? oc[(ia + 3) % 4] : oc[(ia + 1) % 4];
					const int nextB = oc[(ia + 1) % 4] == b ? oc[(ia + 2) % 4] : oc[(ia + 2) % 4];
					const std::pair<int, int> gna = { ga.first + sx, ga.second + sy }, gnb = { gb.first + sx, gb.second + sy };
					auto check = [&](int w, std::pair<int, int> g) {
						auto it = gridOf.find(w);
						if (it == gridOf.end()) gridOf[w] = g;
						else if (it->second != g) ok = false;
					};
					check(nextA, gna);
					check(nextB, gnb);
					visited[o] = 1;
					open.push(o);
				}
			}
		}
		for (char c : visited) if (!c) ok = false;
		int minX = 1 << 30, minY = 1 << 30, maxX = -(1 << 30), maxY = -(1 << 30);
		for (auto& kv : gridOf) { minX = (std::min)(minX, kv.second.first); maxX = (std::max)(maxX, kv.second.first); minY = (std::min)(minY, kv.second.second); maxY = (std::max)(maxY, kv.second.second); }
		const int W = maxX - minX, H = maxY - minY;
		if (ok && W * H != (int)quads.size()) ok = false;
		//座標 → 頂点(完全な格子でなければやめる)
		std::map<std::pair<int, int>, int> at;
		for (auto& kv : gridOf)
		{
			std::pair<int, int> g = { kv.second.first - minX, kv.second.second - minY };
			if (at.count(g)) ok = false;
			at[g] = kv.first;
		}
		if (!ok) { ++irregular; irregularTris += (int)list.size(); for (int t : list) outTri.push_back(tri[t]); continue; }
		const int shortDim = (std::min)(W, H), longDim = (std::max)(W, H);
		++dimHist[{ shortDim, longDim }];

		//元の三角形の向き(格子の右回り/左回り)
		const auto& t0 = tri[list[0]];
		auto gx = [&](int w) { auto g = gridOf[w]; return std::pair<int, int>(g.first - minX, g.second - minY); };
		const auto a0 = gx(t0[0]), b0 = gx(t0[1]), c0 = gx(t0[2]);
		const int orient = (b0.first - a0.first) * (c0.second - a0.second) - (b0.second - a0.second) * (c0.first - a0.first);

		const auto keepX = KeepList(W, W >= H ? lenStep : widStep);
		const auto keepY = KeepList(H, H > W ? lenStep : widStep);
		for (int ix = 0; ix + 1 < (int)keepX.size(); ++ix) for (int iy = 0; iy + 1 < (int)keepY.size(); ++iy)
		{
			const int p00 = at[{ keepX[ix], keepY[iy] }], p10 = at[{ keepX[ix + 1], keepY[iy] }], p11 = at[{ keepX[ix + 1], keepY[iy + 1] }], p01 = at[{ keepX[ix], keepY[iy + 1] }];
			if (orient > 0)
			{
				outTri.push_back({ p00, p10, p11 });
				outTri.push_back({ p00, p11, p01 });
			}
			else
			{
				outTri.push_back({ p00, p11, p10 });
				outTri.push_back({ p00, p01, p11 });
			}
		}
	}
	fprintf(log, "cards=%d irregular=%d (tris %d)\n", (int)cards.size(), irregular, irregularTris);
	for (auto& kv : dimHist) fprintf(log, "  grid %dx%d : %d cards\n", kv.first.first, kv.first.second, kv.second);
	fprintf(log, "out tris=%d (%.1f%%)\n", (int)outTri.size(), 100.0 * outTri.size() / fnum);

	//使う頂点だけ番号を振り直す
	std::vector<int> remap(wnum, -1);
	std::vector<int> outV;
	for (auto& t : outTri) for (int c = 0; c < 3; ++c) if (remap[t[c]] < 0) { remap[t[c]] = (int)outV.size(); outV.push_back(t[c]); }
	fprintf(log, "out verts=%d\n", (int)outV.size());

	std::ofstream out(argv[2]);
	char buf[256];
	for (int k = 0; k <= meshLine; ++k) out << l[k] << "\n";
	const std::string ind = "\t\t\t";
	const int on = (int)outV.size(), of = (int)outTri.size();
	out << ind << on << ";\n";
	for (int k = 0; k < on; ++k) { const Vert& s = v[rep[outV[k]]]; snprintf(buf, sizeof(buf), "%f;%f;%f;%s\n", s.p[0], s.p[1], s.p[2], k + 1 < on ? "," : ";"); out << ind << buf; }
	out << ind << of << ";\n";
	auto writeFaces = [&]() {
		for (int k = 0; k < of; ++k) { snprintf(buf, sizeof(buf), "3;%d,%d,%d;%s\n", remap[outTri[k][0]], remap[outTri[k][1]], remap[outTri[k][2]], k + 1 < of ? "," : ";"); out << ind << buf; }
	};
	writeFaces();
	out << ind << "MeshNormals {\n" << ind << on << ";\n";
	for (int k = 0; k < on; ++k) { const Vert& s = v[rep[outV[k]]]; snprintf(buf, sizeof(buf), "%f;%f;%f;%s\n", s.n[0], s.n[1], s.n[2], k + 1 < on ? "," : ";"); out << ind << buf; }
	out << ind << of << ";\n";
	writeFaces();
	out << ind << "}\n" << ind << "MeshTextureCoords {\n" << ind << on << ";\n";
	for (int k = 0; k < on; ++k) { const Vert& s = v[rep[outV[k]]]; snprintf(buf, sizeof(buf), "%f;%f;%s\n", s.uv[0], s.uv[1], k + 1 < on ? "," : ";"); out << ind << buf; }
	out << ind << "}\n" << ind << "MeshMaterialList {\n" << ind << "1;\n" << ind << of << ";\n";
	for (int k = 0; k < of; ++k) out << ind << "0" << (k + 1 < of ? "," : ";") << "\n";
	out << matRef << "\n" << ind << "}\n";
	//骨ごとのウェイト
	std::vector<std::vector<std::pair<int, float>>> perBone(bones.size());
	for (int k = 0; k < on; ++k) for (auto& w : v[rep[outV[k]]].w) perBone[w.first].push_back({ k, w.second });
	int used = 0;
	for (auto& p : perBone) if (!p.empty()) ++used;
	out << ind << "XSkinMeshHeader {\n" << ind << maxPerVert << ";\n" << ind << maxPerFace << ";\n" << ind << used << ";\n" << ind << "}\n";
	for (int b = 0; b < (int)bones.size(); ++b)
	{
		auto& p = perBone[b];
		if (p.empty()) continue;
		out << ind << "SkinWeights {\n" << ind << "\t" << bones[b].name << "\n" << ind << "\t" << p.size() << ";\n";
		for (size_t k = 0; k < p.size(); ++k) out << ind << "\t" << p[k].first << (k + 1 < p.size() ? "," : ";") << "\n";
		for (size_t k = 0; k < p.size(); ++k) { snprintf(buf, sizeof(buf), "%f%s\n", p[k].second, k + 1 < p.size() ? "," : ";"); out << ind << "\t" << buf; }
		for (auto& m : bones[b].mat) out << m << "\n";
		out << ind << "}\n";
	}
	for (int k = meshEnd; k < (int)l.size(); ++k) out << l[k] << "\n";
	fclose(log);
	return 0;
}
