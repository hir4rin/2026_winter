// アニメを時間を並べて撮る(上段: 斜め前、下段: 横)
// shot.exe 出力.png モデル アニメ名 枚数
#include "DxLib.h"
#include <cstdlib>
#include <cmath>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	const int count = atoi(__argv[4]);
	const int W = 300, H = 300; const int R = 4;
	ChangeWindowMode(TRUE); SetGraphMode(W * count, H * R, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	SetBackgroundColor(60, 60, 75);
	ClearDrawScreen(); DrawBox(0, 0, W * count, H * R, GetColor(60, 60, 75), TRUE); SetGlobalAmbientLight(GetColorF(0.3f, 0.3f, 0.3f, 1.0f));
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	const int h = MV1LoadModel(__argv[2]);
	const int anim = MV1GetAnimIndex(h, __argv[3]);
	if (anim < 0) return 2;
	const int at = MV1AttachAnim(h, anim, -1, FALSE);
	const float len = MV1GetAnimTotalTime(h, anim);
	const VECTOR dirs[R] = { VGet(0.0f, 0.1f, -1.0f), VGet(1.0f, 0.05f, 0.0f), VGet(0.0f, 0.1f, 1.0f), VGet(-1.0f, 0.05f, 0.0f) };
	for (int i = 0; i < count; ++i)
	{
		MV1SetAttachAnimTime(h, at, len * i / (count - 1));
		for (int d = 0; d < R; ++d)
		{
			SetDrawArea(W * i, H * d, W * (i + 1), H * (d + 1));
			SetCameraScreenCenter(W * i + W * 0.5f, H * d + H * 0.5f);
			const VECTOR target = VGet(0, 95, 0);
			SetupCamera_Perspective(2.0f * atanf(tanf(0.35f) * R));//1マスの縦の画角が0.7になるように
			SetCameraNearFar(10, 2000);
			SetCameraPositionAndTarget_UpVecY(VAdd(target, VScale(VNorm(dirs[d]), 230)), target);
			MV1DrawModel(h);
		}
	}
	SetDrawArea(0, 0, W * count, H * R);
	SaveDrawScreenToPNG(0, 0, W * count, H * R, __argv[1]);
	DxLib_End();
	return 0;
}
