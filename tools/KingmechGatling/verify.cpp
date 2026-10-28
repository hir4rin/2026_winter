// 切り出したガトリングを確認する画像を出す
// 使い方: verify.exe ToiletMech.mv1 KingmechGatling.mv1 出力フォルダ
//   gatling_only.png  ガトリングだけ(横から)
//   overlay_*.png     ボスのガトリングを消し、切り出したものを lowerarm_r × オフセットに置いたもの(アニメ中)
#include "DxLib.h"
#include <cmath>
#include <string>

namespace
{
	const VECTOR kOffset = VGet(-52.542f, -5.352f, 3.009f);//extract_log の center
}

static void Shot(const std::string& path, VECTOR eye, VECTOR target)
{
	SetCameraPositionAndTarget_UpVecY(eye, target);
	SetCameraNearFar(1.0f, 5000.0f);
	SaveDrawScreenToPNG(0, 0, 800, 800, path.c_str());
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(800, 800, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (__argc < 4 || DxLib_Init() == -1) return 1;
	const std::string out = std::string(__argv[3]) + "/";
	int boss = MV1LoadModel(__argv[1]);
	int gun = MV1LoadModel(__argv[2]);
	SetDrawScreen(DX_SCREEN_BACK);
	SetBackgroundColor(60, 60, 80);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);

	//ガトリングだけ
	const float views[][3] = { { 0, 30, -200 }, { 200, 30, 0 }, { 120, 120, -120 } };
	for (int i = 0; i < 3; ++i)
	{
		ClearDrawScreen();
		SetCameraPositionAndTarget_UpVecY(VGet(views[i][0], views[i][1], views[i][2]), VGet(0, 0, 0));
		SetCameraNearFar(1.0f, 5000.0f);
		MV1SetMatrix(gun, MGetIdent());
		MV1DrawModel(gun);
		SaveDrawScreenToPNG(0, 0, 800, 800, (out + "gatling_only" + std::to_string(i) + ".png").c_str());
	}

	//ボスに重ねる
	int meshFrame = MV1SearchFrame(boss, "polySurface789");
	int bone = MV1SearchFrame(boss, "lowerarm_r");
	const char* anims[] = { "Idle2", "Attack1", "Attack6_Shoot" };
	for (auto an : anims)
	{
		int ai = MV1GetAnimIndex(boss, an);
		int att = MV1AttachAnim(boss, ai < 0 ? 0 : ai);
		MV1SetAttachAnimTime(boss, att, MV1GetAttachAnimTotalTime(boss, att) * 0.4f);
		//カメラは腕のまわり2方向
		VECTOR arm = MV1GetFramePosition(boss, bone);
		for (int pass = 0; pass < 4; ++pass)
		{
			ClearDrawScreen();
			float r = (pass / 2) * DX_PI_F * 0.5f - DX_PI_F * 0.75f;
			SetCameraPositionAndTarget_UpVecY(VAdd(arm, VGet(cosf(r) * 250, 60, sinf(r) * 250)), arm);
			SetCameraNearFar(1.0f, 5000.0f);
			//偶数: 元のまま / 奇数: 元を消して切り出したものを置く
			MV1SetMeshVisible(boss, MV1GetFrameMesh(boss, meshFrame, 0), pass % 2 == 0);
			MV1DrawModel(boss);
			if (pass % 2 == 1)
			{
				MV1SetMatrix(gun, MMult(MGetTranslate(kOffset), MV1GetFrameLocalWorldMatrix(boss, bone)));
				MV1SetDifColorScale(gun, GetColorF(3.0f, 0.5f, 0.5f, 1.0f));//見分けるため赤く
				MV1SetAmbColorScale(gun, GetColorF(3.0f, 0.5f, 0.5f, 1.0f));
				MV1DrawModel(gun);
			}
			SaveDrawScreenToPNG(0, 0, 800, 800, (out + "overlay_" + an + std::to_string(pass / 2) + (pass % 2 ? "_cut" : "_orig") + ".png").c_str());
		}
		MV1DetachAnim(boss, att);
	}
	DxLib_End();
	return 0;
}
