// ガトリングのメッシュの頂点が、どの骨にどれだけついてくるかを調べる
#include "DxLib.h"
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr; fopen_s(&log, __argv[2], "w");
	int h = MV1LoadModel(__argv[1]);
	int mf = MV1SearchFrame(h, "polySurface789");
	MV1SetupReferenceMesh(h, mf, TRUE);
	MV1_REF_POLYGONLIST r0 = MV1GetReferenceMesh(h, mf, TRUE);
	int vn = r0.VertexNum;
	VECTOR* p0 = new VECTOR[vn];
	for (int v = 0; v < vn; ++v) p0[v] = r0.Vertexs[v].Position;
	fprintf(log, "frame %d verts %d polys %d\n", mf, vn, r0.PolygonNum);
	const char* names[] = { "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "weapon_r", "wrist_inner_r", "wrist_outer_r", "spine_05", "ik_hand_gun" };
	for (auto n : names)
	{
		int b = MV1SearchFrame(h, n);
		if (b < 0) { fprintf(log, "%s none\n", n); continue; }
		VECTOR a = MV1GetFramePosition(h, b);
		MV1SetFrameUserLocalMatrix(h, b, MMult(MV1GetFrameBaseLocalMatrix(h, b), MGetTranslate(VGet(3, 4, 5))));
		VECTOR d = VSub(MV1GetFramePosition(h, b), a);
		MV1RefreshReferenceMesh(h, mf, TRUE);
		MV1_REF_POLYGONLIST r1 = MV1GetReferenceMesh(h, mf, TRUE);
		int hist[11] = {};
		for (int v = 0; v < vn; ++v)
		{
			float s = VDot(VSub(r1.Vertexs[v].Position, p0[v]), d) / VDot(d, d);
			int k = (int)(s * 10 + 0.5f); if (k < 0) k = 0; if (k > 10) k = 10;
			hist[k]++;
		}
		fprintf(log, "%-14s frame %3d pos(%.1f %.1f %.1f) hist:", n, b, a.x, a.y, a.z);
		for (int k = 0; k <= 10; ++k) fprintf(log, " %d", hist[k]);
		fprintf(log, "\n");
		MV1ResetFrameUserLocalMatrix(h, b);
	}
	fclose(log);
	DxLib_End();
	return 0;
}
