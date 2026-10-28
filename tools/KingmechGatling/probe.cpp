// mv1 のフレーム一覧(名前・親・メッシュ数・メッシュの範囲)を log に出す
// 使い方: probe.exe mv1 出力txt
#include "DxLib.h"
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (__argc < 3 || DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, __argv[2], "w");
	int h = MV1LoadModel(__argv[1]);
	fprintf(log, "load %d frames %d meshes %d anims %d\n", h, MV1GetFrameNum(h), MV1GetMeshNum(h), MV1GetAnimNum(h));
	MV1SetupReferenceMesh(h, -1, TRUE);
	for (int f = 0; f < MV1GetFrameNum(h); ++f)
	{
		VECTOR p = MV1GetFramePosition(h, f);
		int mn = MV1GetFrameMeshNum(h, f);
		fprintf(log, "%3d parent %3d mesh %d pos(%.1f %.1f %.1f) %s", f, MV1GetFrameParent(h, f), mn, p.x, p.y, p.z, MV1GetFrameName(h, f));
		if (mn > 0)
		{
			MV1SetupReferenceMesh(h, f, TRUE);
			MV1_REF_POLYGONLIST r = MV1GetReferenceMesh(h, f, TRUE);
			fprintf(log, "  box(%.1f %.1f %.1f)-(%.1f %.1f %.1f) tri %d", r.MinPosition.x, r.MinPosition.y, r.MinPosition.z, r.MaxPosition.x, r.MaxPosition.y, r.MaxPosition.z, r.PolygonNum);
			for (int m = 0; m < mn; ++m) fprintf(log, " [m%d mat %s]", MV1GetFrameMesh(h, f, m), MV1GetMaterialName(h, MV1GetMeshMaterial(h, MV1GetFrameMesh(h, f, m))));
		}
		fprintf(log, "\n");
	}
	fclose(log);
	DxLib_End();
	return 0;
}
