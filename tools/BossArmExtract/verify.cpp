// 切り出した腕にボスのアニメを当てて、ボスと並べて画像にする
// 左: ボス  中: ボスに腕を重ねる(腕は赤)  右: 腕だけ
#include "DxLib.h"
#include <cstdio>
#include <string>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(1200, 600, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);

	int boss = MV1LoadModel("C:/Users/tsyts/GitHub/2026_winter/2026_winter/data/Enemy/BossPirate/BossPirate.mv1");
	int bossB = MV1DuplicateModel(boss);
	int arms[2] = { MV1LoadModel("out/BossArmL.mv1"), MV1LoadModel("out/BossArmR.mv1") };
	int armsB[2] = { arms[0] >= 0 ? MV1DuplicateModel(arms[0]) : -1, arms[1] >= 0 ? MV1DuplicateModel(arms[1]) : -1 };
	const int spine = MV1SearchFrame(boss, "spine_03");

	struct Shot { const char* anim; float time; };
	Shot shots[] = { {"Idle", 60}, {"Sword And Shield Attack", 30}, {"Hit_Large_Combat_F", 12}, {"Block_Loop", 20} };
	int shotNo = 0;
	for (auto& s : shots)
	{
		int idx = MV1GetAnimIndex(boss, s.anim);
		int bossModels[2] = { boss, bossB };
		VECTOR offs[3] = { VGet(-150, 0, 0), VGet(0, 0, 0), VGet(150, 0, 0) };
		for (int i = 0; i < 2; ++i)
		{
			MV1SetMatrix(bossModels[i], MGetTranslate(offs[i]));
			int a = MV1AttachAnim(bossModels[i], idx, -1, FALSE);
			MV1SetAttachAnimTime(bossModels[i], a, s.time);
		}
		int armAttach[2][2];
		for (int k = 0; k < 2; ++k)
		{
			int set[2] = { arms[k], armsB[k] };
			for (int j = 0; j < 2; ++j)
			{
				if (set[j] < 0) { armAttach[k][j] = -1; continue; }
				armAttach[k][j] = MV1AttachAnim(set[j], idx, boss, FALSE);
				MV1SetAttachAnimTime(set[j], armAttach[k][j], s.time);
			}
		}

		ClearDrawScreen();
		DrawBox(0, 0, 1200, 600, GetColor(90, 90, 100), TRUE);
		SetCameraNearFar(10.0f, 3000.0f);
		SetCameraPositionAndTarget_UpVecY(VGet(0, 130, -420), VGet(0, 110, 0));

		MV1DrawModel(boss);//左
		MV1DrawModel(bossB);//中
		for (int k = 0; k < 2; ++k)
		{
			if (arms[k] < 0) continue;
			//中: ボスと同じ行列で重ねる(赤)
			MV1SetMatrix(arms[k], MGetTranslate(offs[1]));
			MV1SetDifColorScale(arms[k], GetColorF(1.0f, 0.2f, 0.2f, 1.0f));
			MV1DrawModel(arms[k]);
			//右: 背骨だけ左のボスの背骨+300に上書き//子の骨がついてくるか
			MV1SetMatrix(armsB[k], MGetIdent());
			MATRIX m = MV1GetFrameLocalWorldMatrix(boss, spine);
			m = MMult(m, MGetTranslate(VGet(300, 0, 0)));
			MV1SetFrameUserLocalWorldMatrix(armsB[k], spine, m);
			MV1DrawModel(armsB[k]);
		}
		DrawString(10, 10, s.anim, GetColor(255, 255, 255));
		std::string png = "verify_" + std::to_string(shotNo++) + ".png";
		SaveDrawScreenToPNG(0, 0, 1200, 600, png.c_str());

		for (int i = 0; i < 2; ++i) MV1DetachAnim(bossModels[i], 0);
		for (int k = 0; k < 2; ++k)
		{
			if (arms[k] >= 0) MV1DetachAnim(arms[k], armAttach[k][0]);
			if (armsB[k] >= 0) MV1DetachAnim(armsB[k], armAttach[k][1]);
		}
	}
	DxLib_End();
	return 0;
}
