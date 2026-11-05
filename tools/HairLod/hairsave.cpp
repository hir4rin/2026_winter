// 書き換えた .x を読み、マテリアルなどを元の mv1 に合わせて、アニメは元の mv1 から持ってきて保存する
// hairsave.exe 元.mv1 新.x 出力.mv1 ログ
#include "DxLib.h"
#include <cstdio>
#include <cstring>
#include <string>

static FILE* g_log = nullptr;

static void DumpMaterial(int h, int m, const char* tag)
{
	COLOR_F d = MV1GetMaterialDifColor(h, m), a = MV1GetMaterialAmbColor(h, m), s = MV1GetMaterialSpcColor(h, m), e = MV1GetMaterialEmiColor(h, m);
	fprintf(g_log, "%s mat %2d %-26s type=%d dif=(%.3f %.3f %.3f %.3f) amb=(%.3f %.3f %.3f) spc=(%.3f %.3f %.3f) emi=(%.3f %.3f %.3f) pow=%.2f blend=%d/%d atest=%d/%d/%d difTex=%d\n",
		tag, m, MV1GetMaterialName(h, m), MV1GetMaterialType(h, m), d.r, d.g, d.b, d.a, a.r, a.g, a.b, s.r, s.g, s.b, e.r, e.g, e.b, MV1GetMaterialSpcPower(h, m),
		MV1GetMaterialDrawBlendMode(h, m), MV1GetMaterialDrawBlendParam(h, m), MV1GetMaterialDrawAlphaTestEnable(h, m), MV1GetMaterialDrawAlphaTestMode(h, m), MV1GetMaterialDrawAlphaTestParam(h, m),
		MV1GetMaterialDifMapTexture(h, m));
}

static void DumpMesh(int h, int m, const char* tag)
{
	fprintf(g_log, "%s mesh %2d mat=%s tris=%d cull=%d vis=%d blend=%d/%d useVDif=%d useVSpc=%d tlists=%d vtype=%d semi=%d\n",
		tag, m, MV1GetMaterialName(h, MV1GetMeshMaterial(h, m)), MV1GetMeshTriangleNum(h, m), MV1GetMeshBackCulling(h, m), MV1GetMeshVisible(h, m),
		MV1GetMeshDrawBlendMode(h, m), MV1GetMeshDrawBlendParam(h, m), MV1GetMeshUseVertDifColor(h, m), MV1GetMeshUseVertSpcColor(h, m), MV1GetMeshTListNum(h, m),
		MV1GetMeshTListNum(h, m) > 0 ? MV1GetTriangleListVertexType(h, MV1GetMeshTList(h, m, 0)) : -1, MV1GetMeshSemiTransState(h, m));
}

static void DumpTexture(int h, int t, const char* tag)
{
	fprintf(g_log, "%s tex %d %s path=%s addr=%d/%d filter=%d\n", tag, t, MV1GetTextureName(h, t), MV1GetTextureColorFilePath(h, t),
		MV1GetTextureAddressModeU(h, t), MV1GetTextureAddressModeV(h, t), MV1GetTextureSampleFilterMode(h, t));
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	SetUseDirect3DVersion(DX_DIRECT3D_11);
	if (DxLib_Init() == -1) return 1;
	fopen_s(&g_log, __argv[4], "w"); setvbuf(g_log, nullptr, _IONBF, 0);
	const int oldH = MV1LoadModel(__argv[1]);
	const int newH = MV1LoadModel(__argv[2]);
	fprintf(g_log, "old=%d new=%d\n", oldH, newH);
	if (oldH < 0 || newH < 0) { fclose(g_log); return 2; }
	fprintf(g_log, "frames %d/%d meshes %d/%d mats %d/%d tex %d/%d anims %d/%d\n", MV1GetFrameNum(oldH), MV1GetFrameNum(newH), MV1GetMeshNum(oldH), MV1GetMeshNum(newH),
		MV1GetMaterialNum(oldH), MV1GetMaterialNum(newH), MV1GetTextureNum(oldH), MV1GetTextureNum(newH), MV1GetAnimNum(oldH), MV1GetAnimNum(newH));
	for (int m = 0; m < MV1GetMaterialNum(oldH); ++m) DumpMaterial(oldH, m, "old");
	for (int m = 0; m < MV1GetMaterialNum(newH); ++m) DumpMaterial(newH, m, "new");
	for (int m = 0; m < MV1GetMeshNum(oldH); ++m) DumpMesh(oldH, m, "old");
	for (int m = 0; m < MV1GetMeshNum(newH); ++m) DumpMesh(newH, m, "new");
	for (int t = 0; t < MV1GetTextureNum(oldH); ++t) DumpTexture(oldH, t, "old");
	for (int t = 0; t < MV1GetTextureNum(newH); ++t) DumpTexture(newH, t, "new");

	//骨の並びと行列
	int frameDiff = 0;
	float maxMatDiff = 0.0f;
	for (int f = 0; f < MV1GetFrameNum(oldH) && f < MV1GetFrameNum(newH); ++f)
	{
		if (strcmp(MV1GetFrameName(oldH, f), MV1GetFrameName(newH, f)) != 0 || MV1GetFrameParent(oldH, f) != MV1GetFrameParent(newH, f))
		{
			if (frameDiff < 10) fprintf(g_log, "frame %d differs: %s / %s\n", f, MV1GetFrameName(oldH, f), MV1GetFrameName(newH, f));
			++frameDiff;
		}
		const MATRIX a = MV1GetFrameBaseLocalMatrix(oldH, f), b = MV1GetFrameBaseLocalMatrix(newH, f);
		for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) { const float d = fabsf(a.m[r][c] - b.m[r][c]); if (d > maxMatDiff) maxMatDiff = d; }
	}
	fprintf(g_log, "frame differs=%d  max base matrix diff=%f\n", frameDiff, maxMatDiff);

	//マテリアルを名前で合わせる
	for (int m = 0; m < MV1GetMaterialNum(newH); ++m)
	{
		const char* name = MV1GetMaterialName(newH, m);
		for (int o = 0; o < MV1GetMaterialNum(oldH); ++o)
		{
			if (strcmp(name, MV1GetMaterialName(oldH, o)) != 0) continue;
			MV1SetMaterialDifColor(newH, m, MV1GetMaterialDifColor(oldH, o));
			MV1SetMaterialAmbColor(newH, m, MV1GetMaterialAmbColor(oldH, o));
			MV1SetMaterialSpcColor(newH, m, MV1GetMaterialSpcColor(oldH, o));
			MV1SetMaterialEmiColor(newH, m, MV1GetMaterialEmiColor(oldH, o));
			MV1SetMaterialSpcPower(newH, m, MV1GetMaterialSpcPower(oldH, o));
			MV1SetMaterialDrawBlendMode(newH, m, MV1GetMaterialDrawBlendMode(oldH, o));
			MV1SetMaterialDrawBlendParam(newH, m, MV1GetMaterialDrawBlendParam(oldH, o));
			MV1SetMaterialDrawAlphaTest(newH, m, MV1GetMaterialDrawAlphaTestEnable(oldH, o), MV1GetMaterialDrawAlphaTestMode(oldH, o), MV1GetMaterialDrawAlphaTestParam(oldH, o));
			break;
		}
	}
	const int ret = MV1SaveModelToMV1File(newH, __argv[3], MV1_SAVETYPE_NORMAL, oldH, TRUE);
	fprintf(g_log, "save ret=%d\n", ret);

	//読み直して確かめる
	const int saved = MV1LoadModel(__argv[3]);
	fprintf(g_log, "saved=%d frames=%d meshes=%d anims=%d\n", saved, saved >= 0 ? MV1GetFrameNum(saved) : -1, saved >= 0 ? MV1GetMeshNum(saved) : -1, saved >= 0 ? MV1GetAnimNum(saved) : -1);
	if (saved >= 0)
	{
		int animDiff = 0;
		for (int a = 0; a < MV1GetAnimNum(oldH) && a < MV1GetAnimNum(saved); ++a)
		{
			if (strcmp(MV1GetAnimName(oldH, a), MV1GetAnimName(saved, a)) != 0 || fabsf(MV1GetAnimTotalTime(oldH, a) - MV1GetAnimTotalTime(saved, a)) > 0.01f)
			{
				if (animDiff < 10) fprintf(g_log, "anim %d differs: %s %.1f / %s %.1f\n", a, MV1GetAnimName(oldH, a), MV1GetAnimTotalTime(oldH, a), MV1GetAnimName(saved, a), MV1GetAnimTotalTime(saved, a));
				++animDiff;
			}
		}
		fprintf(g_log, "anim differs=%d\n", animDiff);
		for (int m = 0; m < MV1GetMaterialNum(saved); ++m) DumpMaterial(saved, m, "saved");
		for (int m = 0; m < MV1GetMeshNum(saved); ++m) DumpMesh(saved, m, "saved");
	}
	fclose(g_log);
	DxLib_End();
	return 0;
}
