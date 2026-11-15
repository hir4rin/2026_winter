// 扉を置く所の床の高さと、左右の壁までの距離を測る
// 使い方: probe_gap.exe 当たり判定.mv1 x z 向き(x|z: 進む向きの軸) 床を探す高さ [x z 向き 高さ ...]
//   進む向きと直角に、床+100の高さで線を左右に当て、壁の位置と幅を出す。天井も上に当てる
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr; fopen_s(&log, "probe_gap_log.txt", "w");
	int h = MV1LoadModel(__argv[1]);
	MV1SetupCollInfo(h, -1);
	for (int a = 2; a + 3 < __argc; a += 4)
	{
		float x = (float)atof(__argv[a]), z = (float)atof(__argv[a + 1]), sy = (float)atof(__argv[a + 3]);
		bool alongX = __argv[a + 2][0] == 'x';
		MV1_COLL_RESULT_POLY d = MV1CollCheck_Line(h, -1, VGet(x, sy, z), VGet(x, sy - 5000, z));
		float fy = d.HitFlag ? d.HitPosition.y : -99999;
		float yy = fy + 100;
		VECTOR side = alongX ? VGet(0, 0, 1) : VGet(1, 0, 0);
		float dist[2];
		for (int s = 0; s < 2; ++s)
		{
			VECTOR from = VGet(x, yy, z), to = VAdd(from, VScale(side, s ? 4000.0f : -4000.0f));
			MV1_COLL_RESULT_POLY r = MV1CollCheck_Line(h, -1, from, to);
			dist[s] = r.HitFlag ? VSize(VSub(r.HitPosition, from)) : 4000;
		}
		MV1_COLL_RESULT_POLY u = MV1CollCheck_Line(h, -1, VGet(x, yy, z), VGet(x, yy + 5000, z));
		float lo = (alongX ? z : x) - dist[0], hi = (alongX ? z : x) + dist[1];
		fprintf(log, "at(%.0f %.0f) along %c floorY %.0f  side %s: %.0f .. %.0f  width %.0f center %.0f  ceil %s\n",
			x, z, alongX ? 'x' : 'z', fy, alongX ? "z" : "x", lo, hi, hi - lo, (lo + hi) / 2,
			u.HitFlag ? "yes" : "none");
		if (u.HitFlag) fprintf(log, "   ceilY %.0f\n", u.HitPosition.y);
	}
	fclose(log); DxLib_End(); return 0;
}
