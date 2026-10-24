// 新しい BossPirate.mv1 に、前の(テクスチャを貼った)mv1 と同じテクスチャ・アルファテスト・色を貼り直す
// マテリアルの並びが前と同じであること(確認済み)
// 使い方: retex.exe 前のmv1(絶対パス)   ※作業フォルダは data/Enemy/BossPirate にして実行する
#include "DxLib.h"
#include <cstdio>
#include <map>
#include <string>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* f = nullptr; fopen_s(&f, __argv[2], "w");
	int oldH = MV1LoadModel(__argv[1]);
	int h = MV1LoadModel("BossPirate.mv1");
	fprintf(f, "old=%d new=%d mats old=%d new=%d\n", oldH, h, MV1GetMaterialNum(oldH), MV1GetMaterialNum(h));
	std::map<std::string, int> added;
	for (int i = 0; i < MV1GetMaterialNum(h); ++i)
	{
		int ot = MV1GetMaterialDifMapTexture(oldH, i);
		if (ot >= 0)
		{
			std::string path = MV1GetTextureColorFilePath(oldH, ot);
			for (auto& c : path) if (c == '\\') c = '/';
			if (!added.count(path))
			{
				//MV1AddTexture の戻り値はテクスチャの番号ではない(成功で0)ので、足した後の最後の番号を使う
				MV1AddTexture(h, MV1GetTextureName(oldH, ot), path.c_str());
				added[path] = MV1GetTextureNum(h) - 1;
			}
			MV1SetMaterialDifMapTexture(h, i, added[path]);
		}
		MV1SetMaterialDrawAlphaTest(h, i, MV1GetMaterialDrawAlphaTestEnable(oldH, i), MV1GetMaterialDrawAlphaTestMode(oldH, i), MV1GetMaterialDrawAlphaTestParam(oldH, i));
		MV1SetMaterialDifColor(h, i, MV1GetMaterialDifColor(oldH, i));
		fprintf(f, "mat[%d] tex=%d alpha=%d\n", i, MV1GetMaterialDifMapTexture(h, i), MV1GetMaterialDrawAlphaTestEnable(h, i));
	}
	int ret = MV1SaveModelToMV1File(h, "BossPirate.mv1");
	fprintf(f, "save ret=%d\n", ret);
	MV1DeleteModel(h);
	int r = MV1LoadModel("BossPirate.mv1");
	fprintf(f, "reload=%d textures=%d anims=%d\n", r, MV1GetTextureNum(r), MV1GetAnimNum(r));
	for (int i = 0; i < MV1GetTextureNum(r); ++i) fprintf(f, " tex[%d] %s\n", i, MV1GetTextureColorFilePath(r, i));
	fclose(f);
	DxLib_End();
	return 0;
}
