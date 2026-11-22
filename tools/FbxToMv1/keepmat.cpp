// .x を mv1 にし直す。マテリアルの色は元の mv1 からコピーする(見た目を変えずにフレームだけ足すとき用)
// 使い方: keepmat.exe 入力.x 元.mv1 出力.mv1   結果(フレーム・形・色の比べ)は keepmat_log.txt
#include "DxLib.h"
#include <cstdio>

namespace
{
	void LogModel(FILE* log, const char* label, int h)
	{
		MV1SetupReferenceMesh(h, -1, TRUE);
		MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(h, -1, TRUE);
		fprintf(log, "[%s] meshes=%d mats=%d frames=%d polys=%d min=(%.3f %.3f %.3f) max=(%.3f %.3f %.3f)\n", label,
			MV1GetMeshNum(h), MV1GetMaterialNum(h), MV1GetFrameNum(h), ref.PolygonNum,
			ref.MinPosition.x, ref.MinPosition.y, ref.MinPosition.z, ref.MaxPosition.x, ref.MaxPosition.y, ref.MaxPosition.z);
		for (int f = 0; f < MV1GetFrameNum(h); ++f)
		{
			VECTOR p = MV1GetFramePosition(h, f);
			fprintf(log, "  frame[%d] %s parent=%d meshes=%d pos=(%.3f %.3f %.3f)\n", f, MV1GetFrameName(h, f),
				MV1GetFrameParent(h, f), MV1GetFrameMeshNum(h, f), p.x, p.y, p.z);
		}
		for (int m = 0; m < MV1GetMaterialNum(h); ++m)
		{
			COLOR_F d = MV1GetMaterialDifColor(h, m), a = MV1GetMaterialAmbColor(h, m), s = MV1GetMaterialSpcColor(h, m), e = MV1GetMaterialEmiColor(h, m);
			fprintf(log, "  mat[%d] %s tex=%d dif=(%.3f %.3f %.3f %.3f) amb=(%.3f %.3f %.3f) spc=(%.3f %.3f %.3f) emi=(%.3f %.3f %.3f) pow=%.3f\n",
				m, MV1GetMaterialName(h, m), MV1GetMaterialDifMapTexture(h, m), d.r, d.g, d.b, d.a, a.r, a.g, a.b, s.r, s.g, s.b, e.r, e.g, e.b,
				MV1GetMaterialSpcPower(h, m));
		}
	}
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, "keepmat_log.txt", "w");
	if (__argc < 4) { fprintf(log, "usage: keepmat.exe in.x old.mv1 out.mv1\n"); fclose(log); DxLib_End(); return 1; }

	int src = MV1LoadModel(__argv[1]);
	int old = MV1LoadModel(__argv[2]);
	if (src < 0 || old < 0) { fprintf(log, "load failed src=%d old=%d\n", src, old); fclose(log); DxLib_End(); return 2; }
	LogModel(log, "old", old);

	if (MV1GetMaterialNum(src) != MV1GetMaterialNum(old)) { fprintf(log, "material num differs\n"); fclose(log); DxLib_End(); return 3; }
	for (int m = 0; m < MV1GetMaterialNum(src); ++m)
	{
		MV1SetMaterialDifColor(src, m, MV1GetMaterialDifColor(old, m));
		MV1SetMaterialAmbColor(src, m, MV1GetMaterialAmbColor(old, m));
		MV1SetMaterialSpcColor(src, m, MV1GetMaterialSpcColor(old, m));
		MV1SetMaterialEmiColor(src, m, MV1GetMaterialEmiColor(old, m));
		MV1SetMaterialSpcPower(src, m, MV1GetMaterialSpcPower(old, m));
	}
	int ret = MV1SaveModelToMV1File(src, __argv[3]);
	fprintf(log, "save %s ret=%d\n", __argv[3], ret);
	MV1DeleteModel(src);
	MV1DeleteModel(old);

	//保存したものを読み直して確かめる
	int saved = MV1LoadModel(__argv[3]);
	if (saved >= 0) LogModel(log, "new", saved);
	fclose(log);
	DxLib_End();
	return ret == 0 ? 0 : 4;
}
