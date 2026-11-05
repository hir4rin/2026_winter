// mv1 をメッシュだけ .x に書き出す
// xdump.exe モデル 出力.x
#include "DxLib.h"
#include <cstdio>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	SetUseDirect3DVersion(DX_DIRECT3D_11);
	MV1SetLoadModelTextureLoad(FALSE);
	if (DxLib_Init() == -1) return 1;
	const int h = MV1LoadModel(__argv[1]);
	if (h < 0) return 2;
	const int ret = MV1SaveModelToXFile(h, __argv[2], MV1_SAVETYPE_MESH);
	DxLib_End();
	return ret == 0 ? 0 : 3;
}
