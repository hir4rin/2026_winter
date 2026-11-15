// mv1 の全マテリアルの色(拡散・環境・反射・発光・反射の強さ)を、別の mv1 のマテリアルと同じにして上書き保存する
// 使い方: copy_mat.exe 元.mv1 元のマテリアル番号[,番号...] 先.mv1 [先.mv1 ...]
//   番号を1つだけ書くと先の全マテリアルに、カンマで並べると先のマテリアル0,1,...に順に写す
//   列車の扉を、ゲームの TrainStage.mv1 の材質(赤さびの波板19・黄黒のテープ10など)と同じ色にするのに使う
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr; fopen_s(&log, "copy_mat_log.txt", "w");
	int src = MV1LoadModel(__argv[1]);
	std::vector<int> list;
	for (const char* p = __argv[2]; *p; )
	{
		list.push_back(atoi(p));
		while (*p && *p != ',') ++p;
		if (*p == ',') ++p;
	}
	for (int sm : list)
	{
		COLOR_F d = MV1GetMaterialDifColor(src, sm), a = MV1GetMaterialAmbColor(src, sm);
		fprintf(log, "src mat%d dif(%.3f %.3f %.3f) amb(%.3f %.3f %.3f)\n", sm, d.r, d.g, d.b, a.r, a.g, a.b);
	}
	for (int i = 3; i < __argc; ++i)
	{
		int h = MV1LoadModel(__argv[i]);
		for (int m = 0; m < MV1GetMaterialNum(h); ++m)
		{
			const int sm = list[m < (int)list.size() ? m : (int)list.size() - 1];
			MV1SetMaterialDifColor(h, m, MV1GetMaterialDifColor(src, sm));
			MV1SetMaterialAmbColor(h, m, MV1GetMaterialAmbColor(src, sm));
			MV1SetMaterialSpcColor(h, m, MV1GetMaterialSpcColor(src, sm));
			MV1SetMaterialEmiColor(h, m, MV1GetMaterialEmiColor(src, sm));
			MV1SetMaterialSpcPower(h, m, MV1GetMaterialSpcPower(src, sm));
		}
		fprintf(log, "%s mats=%d tex=%d save=%d\n", __argv[i], MV1GetMaterialNum(h), MV1GetTextureNum(h), MV1SaveModelToMV1File(h, __argv[i]));
		MV1DeleteModel(h);
	}
	fclose(log); DxLib_End(); return 0;
}
