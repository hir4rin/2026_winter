// mv1 のメッシュ・マテリアルはそのままで、アニメだけ別の mv1 のものに入れ替えて保存する
// animmerge.exe メッシュ.mv1 アニメ.mv1 出力.mv1 ログ
#include "DxLib.h"
#include <cstdio>
#include <cstring>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, __argv[4], "w"); setvbuf(log, nullptr, _IONBF, 0);

	const int meshH = MV1LoadModel(__argv[1]);
	const int animH = MV1LoadModel(__argv[2]);
	fprintf(log, "mesh=%d frames=%d meshes=%d mats=%d tex=%d anims=%d\n", meshH, MV1GetFrameNum(meshH), MV1GetMeshNum(meshH), MV1GetMaterialNum(meshH), MV1GetTextureNum(meshH), MV1GetAnimNum(meshH));
	fprintf(log, "anim=%d frames=%d anims=%d\n", animH, MV1GetFrameNum(animH), MV1GetAnimNum(animH));
	if (meshH < 0 || animH < 0) { fclose(log); return 2; }

	//骨の並びが同じでないと使えない
	int frameDiff = (MV1GetFrameNum(meshH) == MV1GetFrameNum(animH)) ? 0 : 1;
	for (int f = 0; f < MV1GetFrameNum(meshH) && f < MV1GetFrameNum(animH); ++f)
	{
		if (strcmp(MV1GetFrameName(meshH, f), MV1GetFrameName(animH, f)) != 0) ++frameDiff;
	}
	fprintf(log, "frame differs=%d\n", frameDiff);
	if (frameDiff > 0) { fclose(log); return 3; }

	const int ret = MV1SaveModelToMV1File(meshH, __argv[3], MV1_SAVETYPE_NORMAL, animH, TRUE);
	fprintf(log, "save ret=%d\n", ret);

	const int saved = MV1LoadModel(__argv[3]);
	fprintf(log, "saved=%d frames=%d meshes=%d mats=%d tex=%d anims=%d\n", saved, MV1GetFrameNum(saved), MV1GetMeshNum(saved), MV1GetMaterialNum(saved), MV1GetTextureNum(saved), MV1GetAnimNum(saved));

	int tris0 = 0, tris1 = 0;
	for (int m = 0; m < MV1GetMeshNum(meshH); ++m) tris0 += MV1GetMeshTriangleNum(meshH, m);
	for (int m = 0; m < MV1GetMeshNum(saved); ++m) tris1 += MV1GetMeshTriangleNum(saved, m);
	fprintf(log, "tris %d -> %d\n", tris0, tris1);
	for (int m = 0; m < MV1GetMaterialNum(meshH) && m < MV1GetMaterialNum(saved); ++m)
	{
		const COLOR_F a = MV1GetMaterialDifColor(meshH, m), b = MV1GetMaterialDifColor(saved, m);
		const COLOR_F c = MV1GetMaterialAmbColor(meshH, m), d = MV1GetMaterialAmbColor(saved, m);
		fprintf(log, "mat %s dif %.3f/%.3f amb %.3f/%.3f tex %d/%d blend %d/%d\n", MV1GetMaterialName(saved, m), a.r, b.r, c.r, d.r,
			MV1GetMaterialDifMapTexture(meshH, m), MV1GetMaterialDifMapTexture(saved, m), MV1GetMaterialDrawBlendMode(meshH, m), MV1GetMaterialDrawBlendMode(saved, m));
	}
	fclose(log);
	DxLib_End();
	return 0;
}
