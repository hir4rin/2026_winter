// プレイヤーに刀を持たせてアニメを時間で並べて撮る(上: 元のアニメ 横、中: 新しいアニメ 横、下: 新しいアニメ 前)
// swordshot.exe 出力.png Player03.mv1 元のアニメ名 新しいアニメのmv1 刀のmv1 枚数
#include "DxLib.h"
#include <cstdlib>
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
	const int count = atoi(__argv[6]);
	const int W = 220, H = 300, R = 3;
	ChangeWindowMode(TRUE); SetGraphMode(W * count, H * R, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	DrawBox(0, 0, W * count, H * R, GetColor(60, 60, 75), TRUE);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	SetGlobalAmbientLight(GetColorF(0.35f, 0.35f, 0.35f, 1.0f));
	const int player = MV1LoadModel(__argv[2]);
	const int src = MV1LoadModel(__argv[4]);
	const int sword = MV1LoadModel(__argv[5]);
	const int hand = MV1SearchFrame(player, "hand_r");
	const int pelvis = MV1SearchFrame(player, "pelvis");
	for (int row = 0; row < R; ++row)
	{
		int at;
		float len;
		if (row == 0) { const int a = MV1GetAnimIndex(player, __argv[3]); at = MV1AttachAnim(player, a, -1, FALSE); len = MV1GetAnimTotalTime(player, a); }
		else { at = MV1AttachAnim(player, 0, src, FALSE); len = MV1GetAnimTotalTime(src, 0); }
		for (int i = 0; i < count; ++i)
		{
			MV1SetAttachAnimTime(player, at, len * i / (count - 1));
			MV1RefreshCollInfo(player);
			const VECTOR hips = MV1GetFramePosition(player, pelvis);
			const VECTOR target = VGet(0.0f, hips.y, 0.0f);
			const VECTOR dir = (row == 2) ? VGet(0.0f, 0.1f, -1.0f) : VGet(1.0f, 0.05f, 0.0f);
			SetDrawArea(W * i, H * row, W * (i + 1), H * (row + 1));
			SetCameraScreenCenter(W * i + W * 0.5f, H * row + H * 0.5f);
			SetupCamera_Perspective(0.9f);
			SetCameraNearFar(10, 5000);
			SetCameraPositionAndTarget_UpVecY(VAdd(target, VScale(VNorm(dir), 520.0f)), target);
			MV1DrawModel(player);
			MV1SetMatrix(sword, WeaponMat(player, hand));
			MV1DrawModel(sword);
			DrawFormatString(W * i + 4, H * row + 4, GetColor(255, 255, 0), "%d", (int)(len * i / (count - 1) + 0.5f));
		}
		MV1DetachAnim(player, at);
	}
	SetDrawArea(0, 0, W * count, H * R);
	SaveDrawScreenToPNG(0, 0, W * count, H * R, __argv[1]);
	DxLib_End();
	return 0;
}
