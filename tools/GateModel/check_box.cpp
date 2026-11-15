// GateBox.mv1 の面の向きを調べる(外から線を当てて法線を出す)
#include "DxLib.h"
#include <cstdio>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr; fopen_s(&log, "check_box_log.txt", "w");
	int h = MV1LoadModel(__argv[1]);
	MV1SetupCollInfo(h, -1);
	VECTOR from[6] = { VGet(200,0,0), VGet(-200,0,0), VGet(0,200,0), VGet(0,-200,0), VGet(0,0,200), VGet(0,0,-200) };
	for (auto& f : from)
	{
		MV1_COLL_RESULT_POLY r = MV1CollCheck_Line(h, -1, f, VGet(0, 0, 0));
		fprintf(log, "from(%.0f %.0f %.0f) hit=%d normal(%.2f %.2f %.2f)\n", f.x, f.y, f.z, r.HitFlag, r.Normal.x, r.Normal.y, r.Normal.z);
	}
	fclose(log); DxLib_End(); return 0;
}
