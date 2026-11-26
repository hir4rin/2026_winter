// プレイヤーのアニメの1コマのポーズを、刀ごと .efkmodel に書き出す(エフェクトの残像・体の光用)
// posemodel.exe 出力.efkmodel Player03.mv1 刀.mv1 アニメ名 コマ 縮尺(0.05 = 1/20) [法線の向きにふくらませる量(縮尺後)]
// 座標は Effekseer の向き(奥行きを反転)。1/20 で出して 20 倍で再生する
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

static MATRIX WeaponMat(int model, int hand)
{
	MATRIX rot = MGetRotX(90.0f * DX_PI_F / 180.0f);
	rot = MMult(rot, MGetRotY(90.0f * DX_PI_F / 180.0f + DX_PI_F / 2));
	rot = MMult(rot, MGetRotZ(0.0f - DX_PI_F / 2));
	return MMult(MMult(MGetScale(VGet(0.6f, 0.6f, 0.6f)), rot), MV1GetFrameLocalWorldMatrix(model, hand));
}

struct V { VECTOR p, n; };

static void AddMesh(int model, float s, float inflate, std::vector<V>& vs, std::vector<int>& fs)
{
	MV1SetupReferenceMesh(model, -1, TRUE);
	MV1RefreshReferenceMesh(model, -1, TRUE);
	const MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(model, -1, TRUE);
	const int base = (int)vs.size();
	for (int i = 0; i < ref.VertexNum; ++i)
	{
		//Effekseer は DxLib と奥行きが逆
		const VECTOR n = VNorm(ref.Vertexs[i].Normal), p = VAdd(VScale(ref.Vertexs[i].Position, s), VScale(n, inflate));
		vs.push_back({ VGet(p.x, p.y, -p.z), VGet(n.x, n.y, -n.z) });
	}
	for (int i = 0; i < ref.PolygonNum; ++i)
	{
		const int* v = ref.Polygons[i].VIndex;
		fs.push_back(base + v[0]); fs.push_back(base + v[1]); fs.push_back(base + v[2]);
	}
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(64, 64, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	const float s = (float)atof(__argv[6]);
	const float inflate = (__argc > 7) ? (float)atof(__argv[7]) : 0.0f;
	const int player = MV1LoadModel(__argv[2]);
	const int sword = MV1LoadModel(__argv[3]);
	const int anim = MV1GetAnimIndex(player, __argv[4]);
	const int at = MV1AttachAnim(player, anim, -1, FALSE);
	MV1SetAttachAnimTime(player, at, (float)atof(__argv[5]));
	MV1SetMatrix(player, MGetIdent());
	const int hand = MV1SearchFrame(player, "hand_r");
	MV1SetMatrix(sword, WeaponMat(player, hand));

	std::vector<V> vs; std::vector<int> fs;
	AddMesh(player, s, inflate, vs, fs);
	AddMesh(sword, s, inflate, vs, fs);

	FILE* fp = fopen(__argv[1], "wb");
	const int head[4] = { 5, 0, 1, 1 };
	fwrite(&head[0], 4, 1, fp); const float one = 1.0f; fwrite(&one, 4, 1, fp); fwrite(&head[2], 4, 2, fp);
	const int vn = (int)vs.size(); fwrite(&vn, 4, 1, fp);
	for (const auto& v : vs)
	{
		const float f[14] = { v.p.x, v.p.y, v.p.z, v.n.x, v.n.y, v.n.z, 0, 1, 0, 1, 0, 0, 0, 0 };
		fwrite(f, 4, 14, fp);
		const unsigned char c[4] = { 255, 255, 255, 255 };
		fwrite(c, 1, 4, fp);
	}
	const int fn = (int)fs.size() / 3; fwrite(&fn, 4, 1, fp);
	fwrite(fs.data(), 4, fs.size(), fp);
	fclose(fp);

	FILE* log = fopen("posemodel_log.txt", "w");
	auto P = [&](const char* name, VECTOR p) { fprintf(log, "%s %.2f %.2f %.2f\n", name, p.x * s, p.y * s, -p.z * s); };
	P("head", MV1GetFramePosition(player, MV1SearchFrame(player, "head")));
	P("pelvis", MV1GetFramePosition(player, MV1SearchFrame(player, "pelvis")));
	P("hand_r", MV1GetFramePosition(player, hand));
	VECTOR mn = VGet(1e9f, 1e9f, 1e9f), mx = VGet(-1e9f, -1e9f, -1e9f);
	for (const auto& v : vs) { mn = VGet(min(mn.x, v.p.x), min(mn.y, v.p.y), min(mn.z, v.p.z)); mx = VGet(max(mx.x, v.p.x), max(mx.y, v.p.y), max(mx.z, v.p.z)); }
	fprintf(log, "min %.2f %.2f %.2f\nmax %.2f %.2f %.2f\nverts %d faces %d\n", mn.x, mn.y, mn.z, mx.x, mx.y, mx.z, vn, fn);
	fclose(log);
	DxLib_End();
	return 0;
}
