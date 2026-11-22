// 2つのモデルの同じ名前のアニメを比べる(骨の位置の差の最大)。Bを省くとAの腰の動きだけ出す
// animcmp.exe ログ A.mv1 [B.mv1]
#include "DxLib.h"
#include <cstdio>
#include <cmath>

// アニメを1つだけ付けて、指定フレームの姿勢にする
static int Pose(int h, int anim, float t, int attach)
{
	if (attach >= 0) MV1DetachAnim(h, attach);
	attach = MV1AttachAnim(h, anim, -1, FALSE);
	MV1SetAttachAnimTime(h, attach, t);
	MV1RefreshCollInfo(h, -1);
	return attach;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, __argv[1], "w"); setvbuf(log, nullptr, _IONBF, 0);
	const int a = MV1LoadModel(__argv[2]);
	const int b = __argc > 3 ? MV1LoadModel(__argv[3]) : -1;
	const int hipsA = MV1SearchFrame(a, "mixamorig:Hips");
	fprintf(log, "A anims=%d B anims=%d hips=%d\n", MV1GetAnimNum(a), b >= 0 ? MV1GetAnimNum(b) : -1, hipsA);
	int atA = -1, atB = -1;
	for (int i = 0; i < MV1GetAnimNum(a); ++i)
	{
		const char* name = MV1GetAnimName(a, i);
		const float len = MV1GetAnimTotalTime(a, i);
		atA = Pose(a, i, 0.0f, atA);
		const VECTOR h0 = MV1GetFramePosition(a, hipsA);
		atA = Pose(a, i, len, atA);
		const VECTOR h1 = MV1GetFramePosition(a, hipsA);
		fprintf(log, "%-55s len=%6.1f hips0=(%6.1f %6.1f %6.1f) hipsEnd=(%6.1f %6.1f %6.1f)", name, len, h0.x, h0.y, h0.z, h1.x, h1.y, h1.z);
		const int j = b >= 0 ? MV1GetAnimIndex(b, name) : -1;
		if (j >= 0)
		{
			float maxD = 0.0f, maxAt = 0.0f; int maxF = -1;
			for (float t = 0.0f; t <= len; t += 1.0f)
			{
				atA = Pose(a, i, t, atA);
				atB = Pose(b, j, t, atB);
				for (int f = 0; f < MV1GetFrameNum(a); ++f)
				{
					const VECTOR p = MV1GetFramePosition(a, f), q = MV1GetFramePosition(b, f);
					const float d = VSize(VSub(p, q));
					if (d > maxD) { maxD = d; maxAt = t; maxF = f; }
				}
			}
			fprintf(log, " lenB=%6.1f maxDiff=%.3f at t=%.0f %s", MV1GetAnimTotalTime(b, j), maxD, maxAt, maxF >= 0 ? MV1GetFrameName(a, maxF) : "");
		}
		else if (b >= 0) fprintf(log, " (B none)");
		fprintf(log, "\n");
	}
	fclose(log);
	DxLib_End();
	return 0;
}
