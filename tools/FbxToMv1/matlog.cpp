// mv1 のマテリアルの色と頂点の種類を書き出す(調べる用)
// 使い方: matlog.exe モデル.mv1 [...]  結果は matlog.txt
#include "DxLib.h"
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, "matlog.txt", "w");
	for (int i = 1; i < __argc; ++i)
	{
		int h = MV1LoadModel(__argv[i]);
		fprintf(log, "%s mats=%d\n", __argv[i], MV1GetMaterialNum(h));
		for (int m = 0; m < MV1GetMaterialNum(h) && m < 8; ++m)
		{
			COLOR_F d = MV1GetMaterialDifColor(h, m), a = MV1GetMaterialAmbColor(h, m), e = MV1GetMaterialEmiColor(h, m), s = MV1GetMaterialSpcColor(h, m);
			fprintf(log, "  [%d] %s dif=(%.2f %.2f %.2f) amb=(%.2f %.2f %.2f) spc=(%.2f) emi=(%.2f %.2f %.2f) tex=%d\n", m, MV1GetMaterialName(h, m),
				d.r, d.g, d.b, a.r, a.g, a.b, s.r, e.r, e.g, e.b, MV1GetMaterialDifMapTexture(h, m));
		}
	}
	fclose(log);
	DxLib_End();
	return 0;
}
