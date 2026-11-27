// プレイヤーに刀(右手)とバズーカ(左手で肩に担ぐ)を持たせてアニメを時間で並べて撮る(上: 左横、中: 前、下: 斜め上)
// bazshot.exe 出力.png Player03.mv1 アニメのmv1 刀のmv1 RPG7.mv1 枚数 [アニメ番号]
// バズーカの向き: 肩の上(upperarm_l ＋上へ kShoulderUp)から手の上(hand_l ＋上へ kGripUp)へ。原点(引き金)は手
#include "DxLib.h"
#include <cstdlib>

namespace
{
	constexpr float kShoulderUp = 9.0f;// 筒が乗る肩の高さ
	constexpr float kGripUp = 9.0f;    // 握りから筒までの高さ
}

static MATRIX WeaponMat(int model, int hand)
{
	MATRIX rot = MGetRotX(90.0f * DX_PI_F / 180.0f);
	rot = MMult(rot, MGetRotY(90.0f * DX_PI_F / 180.0f + DX_PI_F / 2));
	rot = MMult(rot, MGetRotZ(0.0f - DX_PI_F / 2));
	return MMult(MMult(MGetScale(VGet(0.6f, 0.6f, 0.6f)), rot), MV1GetFrameLocalWorldMatrix(model, hand));
}

static MATRIX BazookaMat(int model, int hand, int upperArm, int spine, int neck)
{
	const VECTOR up0 = VNorm(VSub(MV1GetFramePosition(model, neck), MV1GetFramePosition(model, spine)));
	const VECTOR grip = MV1GetFramePosition(model, hand);
	const VECTOR back = VAdd(MV1GetFramePosition(model, upperArm), VScale(up0, kShoulderUp));
	const VECTOR front = VAdd(grip, VScale(up0, kGripUp));
	const VECTOR z = VNorm(VSub(front, back));
	const VECTOR x = VNorm(VCross(up0, z));
	const VECTOR y = VCross(z, x);
	MATRIX m = MGetIdent();
	m.m[0][0] = x.x; m.m[0][1] = x.y; m.m[0][2] = x.z;
	m.m[1][0] = y.x; m.m[1][1] = y.y; m.m[1][2] = y.z;
	m.m[2][0] = z.x; m.m[2][1] = z.y; m.m[2][2] = z.z;
	m.m[3][0] = grip.x; m.m[3][1] = grip.y; m.m[3][2] = grip.z;
	return m;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	const int count = atoi(__argv[6]);
	int animNo = 0;//番号かアニメ名
	const int W = 220, H = 300, R = 3;
	ChangeWindowMode(TRUE); SetGraphMode(W * count, H * R, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	DrawBox(0, 0, W * count, H * R, GetColor(60, 60, 75), TRUE);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	SetGlobalAmbientLight(GetColorF(0.35f, 0.35f, 0.35f, 1.0f));
	const int player = MV1LoadModel(__argv[2]);
	const int src = MV1LoadModel(__argv[3]);
	const int sword = MV1LoadModel(__argv[4]);
	const int baz = MV1LoadModel(__argv[5]);
	if (__argc > 7) animNo = (__argv[7][0] >= '0' && __argv[7][0] <= '9') ? atoi(__argv[7]) : MV1GetAnimIndex(src, __argv[7]);
	const int handR = MV1SearchFrame(player, "hand_r");
	const int handL = MV1SearchFrame(player, "hand_l");
	const int upperL = MV1SearchFrame(player, "upperarm_l");
	const int spine = MV1SearchFrame(player, "spine_03");
	const int neck = MV1SearchFrame(player, "neck_01");
	const int pelvis = MV1SearchFrame(player, "pelvis");
	const int at = MV1AttachAnim(player, animNo, src, FALSE);
	const float len = MV1GetAnimTotalTime(src, animNo);
	for (int row = 0; row < R; ++row)
	{
		for (int i = 0; i < count; ++i)
		{
			const float t = len * i / (count - 1);
			MV1SetAttachAnimTime(player, at, t);
			MV1RefreshCollInfo(player);
			const VECTOR hips = MV1GetFramePosition(player, pelvis);
			const VECTOR target = VGet(hips.x, hips.y + 20.0f, hips.z);
			const VECTOR dir = (row == 0) ? VGet(1.0f, 0.05f, 0.0f) : (row == 1) ? VGet(0.0f, 0.1f, -1.0f) : VGet(0.8f, 0.6f, -0.6f);
			SetDrawArea(W * i, H * row, W * (i + 1), H * (row + 1));
			SetCameraScreenCenter(W * i + W * 0.5f, H * row + H * 0.5f);
			SetupCamera_Perspective(0.9f);
			SetCameraNearFar(10, 5000);
			SetCameraPositionAndTarget_UpVecY(VAdd(target, VScale(VNorm(dir), 560.0f)), target);
			MV1DrawModel(player);
			MV1SetMatrix(sword, WeaponMat(player, handR));
			MV1DrawModel(sword);
			MV1SetMatrix(baz, BazookaMat(player, handL, upperL, spine, neck));
			MV1DrawModel(baz);
			DrawFormatString(W * i + 4, H * row + 4, GetColor(255, 255, 0), "%d", (int)(t + 0.5f));
		}
	}
	SetDrawArea(0, 0, W * count, H * R);
	SaveDrawScreenToPNG(0, 0, W * count, H * R, __argv[1]);
	DxLib_End();
	return 0;
}
