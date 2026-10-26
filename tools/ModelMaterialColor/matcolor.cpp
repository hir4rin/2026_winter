// mv1 の全マテリアルの色を書き換えて上書き保存する
// ディフューズ・スペキュラ = kDif / kSpc、アンビエント = kAmb(RGBだけ。アルファはそのまま)
// 使い方: matcolor.exe ログ mv1 mv1 ...   ※パスは作業フォルダ(2026_winter/2026_winter)から
#include "DxLib.h"
#include <cstdio>

namespace
{
	constexpr float kDif = 0.1f;
	constexpr float kSpc = 0.1f;
	constexpr float kAmb = 0.9f;
}

static COLOR_F WithRgb(COLOR_F c, float v)
{
	c.r = v; c.g = v; c.b = v;
	return c;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* f = nullptr; fopen_s(&f, __argv[1], "w");
	for (int a = 2; a < __argc; ++a)
	{
		const char* path = __argv[a];
		int h = MV1LoadModel(path);
		if (h < 0) { fprintf(f, "[NG] load %s\n", path); continue; }
		const int matNum = MV1GetMaterialNum(h);
		const int animNum = MV1GetAnimNum(h);
		const int texNum = MV1GetTextureNum(h);
		for (int i = 0; i < matNum; ++i)
		{
			COLOR_F dif = MV1GetMaterialDifColor(h, i);
			COLOR_F spc = MV1GetMaterialSpcColor(h, i);
			COLOR_F amb = MV1GetMaterialAmbColor(h, i);
			fprintf(f, "  %s mat[%d] before dif(%.2f,%.2f,%.2f,%.2f) spc(%.2f,%.2f,%.2f) amb(%.2f,%.2f,%.2f)\n",
				path, i, dif.r, dif.g, dif.b, dif.a, spc.r, spc.g, spc.b, amb.r, amb.g, amb.b);
			MV1SetMaterialDifColor(h, i, WithRgb(dif, kDif));
			MV1SetMaterialSpcColor(h, i, WithRgb(spc, kSpc));
			MV1SetMaterialAmbColor(h, i, WithRgb(amb, kAmb));
		}
		//圧縮しない(法線8bit・位置16bit・アニメ16bitをオフ)
		int ret = MV1SaveModelToMV1File(h, path, MV1_SAVETYPE_NORMAL, -1, TRUE, 0, 0, 0, 0);
		MV1DeleteModel(h);

		//読み直して確認
		int r = MV1LoadModel(path);
		bool ok = ret == 0 && r >= 0 && MV1GetMaterialNum(r) == matNum && MV1GetAnimNum(r) == animNum && MV1GetTextureNum(r) == texNum;
		if (r >= 0 && matNum > 0)
		{
			COLOR_F dif = MV1GetMaterialDifColor(r, 0);
			COLOR_F amb = MV1GetMaterialAmbColor(r, 0);
			fprintf(f, "  after mat[0] dif(%.2f,%.2f,%.2f,%.2f) amb(%.2f,%.2f,%.2f)\n", dif.r, dif.g, dif.b, dif.a, amb.r, amb.g, amb.b);
		}
		fprintf(f, "[%s] %s save=%d mats=%d anims=%d textures=%d\n", ok ? "OK" : "NG", path, ret, matNum, animNum, texNum);
		if (r >= 0) MV1DeleteModel(r);
	}
	fclose(f);
	DxLib_End();
	return 0;
}
