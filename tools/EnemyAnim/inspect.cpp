// mv1/fbx の骨・アニメ・メッシュを書き出す
// inspect.exe ログ モデル...
#include "DxLib.h"
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, __argv[1], "w");
	for (int i = 2; i < __argc; ++i)
	{
		const int h = MV1LoadModel(__argv[i]);
		int tris = 0;
		for (int m = 0; m < MV1GetMeshNum(h); ++m) tris += MV1GetMeshTriangleNum(h, m);
		fprintf(log, "== %s h=%d frames=%d meshes=%d tris=%d mats=%d anims=%d\n", __argv[i], h, MV1GetFrameNum(h), MV1GetMeshNum(h), tris, MV1GetMaterialNum(h), MV1GetAnimNum(h));
		for (int f = 0; f < MV1GetFrameNum(h); ++f)
		{
			const VECTOR p = MV1GetFramePosition(h, f);
			fprintf(log, "  F%d %s parent=%d pos=%.1f,%.1f,%.1f\n", f, MV1GetFrameName(h, f), MV1GetFrameParent(h, f), p.x, p.y, p.z);
		}
		for (int a = 0; a < MV1GetAnimNum(h); ++a)
			fprintf(log, "  A%d %s len=%.2f\n", a, MV1GetAnimName(h, a), MV1GetAnimTotalTime(h, a));
	}
	fclose(log);
	DxLib_End();
	return 0;
}
