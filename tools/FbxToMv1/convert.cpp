// FBX などを DxLib で読み込んで mv1 に保存する
// 使い方: convert.exe [flat] 入力.fbx 出力.mv1 [入力2 出力2 ...]
//   flat: キャラ・デモステージと同じく、拡散光を弱く(色×0.1)・環境光を強く(色×0.9)・反射0.1にする(色は .x の拡散色を使う)
// 結果(外形の大きさ・メッシュ数)は convert_log.txt に書く
#include "DxLib.h"
#include <cstdio>
#include <cstring>

namespace
{
	constexpr float kFlatDifRate = 0.1f;//flatのときの拡散色の倍率(System.cppで読む体・ステージのモデルと同じ)
	constexpr float kFlatAmbRate = 0.9f;//flatのときの環境色の倍率
	constexpr float kFlatSpc = 0.1f;//flatのときの反射色
	constexpr float kDefaultAmb = 0.08f;//flatでないとき、環境色が無いマテリアルに入れる値
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, "convert_log.txt", "w");
	int result = 0;
	const bool flat = __argc > 1 && strcmp(__argv[1], "flat") == 0;
	for (int i = flat ? 2 : 1; i + 1 < __argc; i += 2)
	{
		int h = MV1LoadModel(__argv[i]);
		if (h < 0)
		{
			fprintf(log, "load failed %s\n", __argv[i]);
			result = 2;
			continue;
		}
		MV1SetupReferenceMesh(h, -1, TRUE);
		MV1_REF_POLYGONLIST ref = MV1GetReferenceMesh(h, -1, TRUE);
		fprintf(log, "%s meshes=%d mats=%d polys=%d min=(%.1f %.1f %.1f) max=(%.1f %.1f %.1f)\n", __argv[i],
			MV1GetMeshNum(h), MV1GetMaterialNum(h), ref.PolygonNum,
			ref.MinPosition.x, ref.MinPosition.y, ref.MinPosition.z, ref.MaxPosition.x, ref.MaxPosition.y, ref.MaxPosition.z);
		for (int m = 0; m < MV1GetMaterialNum(h); ++m)
		{
			COLOR_F dif = MV1GetMaterialDifColor(h, m);
			if (flat)
			{
				MV1SetMaterialDifColor(h, m, GetColorF(dif.r * kFlatDifRate, dif.g * kFlatDifRate, dif.b * kFlatDifRate, dif.a));
				MV1SetMaterialAmbColor(h, m, GetColorF(dif.r * kFlatAmbRate, dif.g * kFlatAmbRate, dif.b * kFlatAmbRate, 1.0f));
				MV1SetMaterialSpcColor(h, m, GetColorF(kFlatSpc, kFlatSpc, kFlatSpc, 1.0f));
			}
			else
			{
				//.x には環境光の色が無く、横向きの面が真っ黒になるので少しだけ入れておく(多いと白っぽくなる)
				COLOR_F amb = MV1GetMaterialAmbColor(h, m);
				if (amb.r + amb.g + amb.b <= 0.0f) MV1SetMaterialAmbColor(h, m, GetColorF(kDefaultAmb, kDefaultAmb, kDefaultAmb, 1.0f));
			}
			fprintf(log, "  mat[%d] %s tex=%d dif=(%.2f %.2f %.2f)\n", m, MV1GetMaterialName(h, m), MV1GetMaterialDifMapTexture(h, m), dif.r, dif.g, dif.b);
		}
		int ret = MV1SaveModelToMV1File(h, __argv[i + 1]);
		fprintf(log, "save %s ret=%d\n", __argv[i + 1], ret);
		if (ret != 0) result = 3;
		MV1DeleteModel(h);
	}
	fclose(log);
	DxLib_End();
	return result;
}
