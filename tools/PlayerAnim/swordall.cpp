// アニメのmv1の全部のアニメを、プレイヤーに刀を持たせて横から並べて撮る(1行=1本)
// swordall.exe 出力.png Player03.mv1 アニメのmv1 刀のmv1 枚数 [向き 0=横 1=前]
#include "DxLib.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>

static MATRIX WeaponMat(int model, int hand)
{
	MATRIX rot = MGetRotX(90.0f * DX_PI_F / 180.0f);
	rot = MMult(rot, MGetRotY(90.0f * DX_PI_F / 180.0f + DX_PI_F / 2));
	rot = MMult(rot, MGetRotZ(0.0f - DX_PI_F / 2));
	return MMult(MMult(MGetScale(VGet(0.6f, 0.6f, 0.6f)), rot), MV1GetFrameLocalWorldMatrix(model, hand));
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	const int count = atoi(__argv[5]);
	const int front = (__argc > 6) ? atoi(__argv[6]) : 0;
	const float rangeStart = (__argc > 8) ? (float)atof(__argv[7]) : -1.0f;//撮るフレームの範囲(省くと全部)
	const float rangeEnd = (__argc > 8) ? (float)atof(__argv[8]) : -1.0f;
	const int W = 170, H = 230, LW = 150;
	ChangeWindowMode(TRUE); SetGraphMode(320, 240, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	int src = MV1LoadModel(__argv[3]);
	const int rows = MV1GetAnimNum(src);
	const int sw = LW + W * count, sh = H * rows;
	SetGraphMode(sw, sh, 32);//ここでモデルが消えるので後で読み直す
	src = MV1LoadModel(__argv[3]);
	const int player = MV1LoadModel(__argv[2]);
	const int sword = MV1LoadModel(__argv[4]);
	const int hand = MV1SearchFrame(player, "hand_r");
	const int pelvis = MV1SearchFrame(player, "pelvis");
	const int screen = MakeScreen(sw, sh, FALSE);
	SetDrawScreen(screen);
	DrawBox(0, 0, sw, sh, GetColor(60, 60, 75), TRUE);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	SetGlobalAmbientLight(GetColorF(0.35f, 0.35f, 0.35f, 1.0f));
	for (int row = 0; row < rows; ++row)
	{
		const int at = MV1AttachAnim(player, row, src, FALSE);
		const float len = MV1GetAnimTotalTime(src, row);
		SetDrawArea(0, 0, sw, sh);
		DrawFormatString(4, H * row + 8, GetColor(255, 255, 255), "%d %s", row, MV1GetAnimName(src, row));
		DrawFormatString(4, H * row + 28, GetColor(200, 200, 200), "len %.0f", len);
		for (int i = 0; i < count; ++i)
		{
			const float t = (rangeStart >= 0.0f) ? rangeStart + (rangeEnd - rangeStart) * i / (count - 1) : len * i / (count - 1);
			MV1SetAttachAnimTime(player, at, t);
			const VECTOR hips = MV1GetFramePosition(player, pelvis);
			const VECTOR target = VGet(0.0f, hips.y, 0.0f);
			const VECTOR dir = front ? VGet(0.0f, 0.1f, -1.0f) : VGet(1.0f, 0.05f, 0.0f);
			const int x0 = LW + W * i, y0 = H * row;
			SetDrawArea(x0, y0, x0 + W, y0 + H);
			SetCameraScreenCenter(x0 + W * 0.5f, y0 + H * 0.5f);
			SetupCamera_Perspective(2.0f * atanf(tanf(0.5f) * rows));//1マスの縦の画角が1.0
			SetCameraNearFar(10, 5000);
			SetCameraPositionAndTarget_UpVecY(VAdd(target, VScale(VNorm(dir), 480.0f)), target);
			ClearDrawScreenZBuffer();
			MV1DrawModel(player);
			MV1SetMatrix(sword, WeaponMat(player, hand));
			MV1DrawModel(sword);
			DrawFormatString(x0 + 3, y0 + 3, GetColor(255, 255, 0), "%d", (int)(t + 0.5f));
		}
		MV1DetachAnim(player, at);
	}
	SetDrawArea(0, 0, sw, sh);
	SaveDrawScreenToPNG(0, 0, sw, sh, __argv[1]);
	DxLib_End();
	return 0;
}
