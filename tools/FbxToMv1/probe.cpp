// mv1 の当たり判定で、真下へ線を引いて当たった面の位置と法線を調べる
// 使い方: probe.exe モデル.mv1 x z [x z ...]  結果は probe_log.txt
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, "probe_log.txt", "w");
	int h = MV1LoadModel(__argv[1]);
	MV1SetupCollInfo(h, -1);
	for (int i = 2; i + 1 < __argc; i += 2)
	{
		float x = static_cast<float>(atof(__argv[i])), z = static_cast<float>(atof(__argv[i + 1]));
		MV1_COLL_RESULT_POLY hit = MV1CollCheck_Line(h, -1, VGet(x, 2000.0f, z), VGet(x, -2000.0f, z));
		fprintf(log, "(%.0f,%.0f) hit=%d pos=(%.1f %.1f %.1f) normal=(%.2f %.2f %.2f)\n", x, z, hit.HitFlag,
			hit.HitPosition.x, hit.HitPosition.y, hit.HitPosition.z, hit.Normal.x, hit.Normal.y, hit.Normal.z);
	}
	fclose(log);
	DxLib_End();
	return 0;
}
