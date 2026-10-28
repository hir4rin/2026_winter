// mv1 を撮って PNG に出す(確認用)
// 使い方: shot.exe mv1 出力png 見せるメッシュ(-1=全部) 角度(度) [注視y 距離]
#include "DxLib.h"
#include <cmath>
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(800, 800, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (__argc < 5 || DxLib_Init() == -1) return 1;
	int h = MV1LoadModel(__argv[1]);
	int show = atoi(__argv[3]);
	float deg = (float)atof(__argv[4]);
	float ty = __argc > 5 ? (float)atof(__argv[5]) : 100.0f;
	float dist = __argc > 6 ? (float)atof(__argv[6]) : 350.0f;
	if (show >= 0) for (int m = 0; m < MV1GetMeshNum(h); ++m) MV1SetMeshVisible(h, m, m == show);
	SetDrawScreen(DX_SCREEN_BACK);
	SetBackgroundColor(60, 60, 80);
	ClearDrawScreen();
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	float r = deg * DX_PI_F / 180.0f;
	SetCameraPositionAndTarget_UpVecY(VGet(sinf(r) * dist, ty, -cosf(r) * dist), VGet(0, ty, 0));
	SetCameraNearFar(1.0f, 5000.0f);
	MV1DrawModel(h);
	SaveDrawScreenToPNG(0, 0, 800, 800, __argv[2]);
	DxLib_End();
	return 0;
}
