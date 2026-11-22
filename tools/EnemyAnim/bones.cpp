// アニメの指定フレームの主な骨の位置を出す
// bones.exe ログ モデル アニメ名 フレーム...
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	FILE* log = nullptr;
	fopen_s(&log, __argv[1], "w"); setvbuf(log, nullptr, _IONBF, 0);
	const int h = MV1LoadModel(__argv[2]);
	const int anim = MV1GetAnimIndex(h, __argv[3]);
	const int at = MV1AttachAnim(h, anim, -1, FALSE);
	const char* names[] = { "mixamorig:Hips", "mixamorig:Head", "mixamorig:LeftUpLeg", "mixamorig:LeftLeg", "mixamorig:LeftFoot", "mixamorig:RightFoot", "mixamorig:LeftHand", "mixamorig:RightHand" };
	for (int i = 4; i < __argc; ++i)
	{
		MV1SetAttachAnimTime(h, at, static_cast<float>(atof(__argv[i])));
		fprintf(log, "t=%s\n", __argv[i]);
		for (auto n : names)
		{
			const VECTOR p = MV1GetFramePosition(h, MV1SearchFrame(h, n));
			fprintf(log, "  %-22s %7.1f %7.1f %7.1f\n", n, p.x, p.y, p.z);
		}
	}
	fclose(log);
	DxLib_End();
	return 0;
}
