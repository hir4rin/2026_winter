// 2つのモデルに同じアニメを付けて全部のアニメで見比べる
// hairanim.exe 元.mv1 新.mv1 ログ 出力の頭
#include "DxLib.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct Shot { const char* name; VECTOR dir; };
static const Shot kShots[] = { { "back", { 0, 0.15f, 1 } }, { "side", { 1, 0.15f, 0 } }, { "front", { 0, 0.15f, -1 } } };
static const float kRates[] = { 0.15f, 0.4f, 0.65f, 0.9f };

static void DrawPose(int h, int head, int anim, float rate, const Shot& s)
{
	const int attach = MV1AttachAnim(h, anim, -1, FALSE);
	MV1SetAttachAnimTime(h, attach, MV1GetAttachAnimTotalTime(h, attach) * rate);
	const VECTOR target = MV1GetFramePosition(h, head);
	ClearDrawScreen();
	SetCameraPositionAndTarget_UpVecY(VAdd(target, VScale(VNorm(s.dir), 130.0f)), target);
	MV1DrawModel(h);
	MV1DetachAnim(h, attach);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(1280, 720, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	SetUseDirect3DVersion(DX_DIRECT3D_11);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	SetBackgroundColor(90, 90, 100);
	SetLightDirection(VGet(-0.3f, -0.6f, 0.8f));
	SetCameraNearFar(5.0f, 2000.0f);
	SetupCamera_Perspective(45.0f * DX_PI_F / 180.0f);
	FILE* fp = nullptr;
	fopen_s(&fp, __argv[3], "w");
	const int a = MV1LoadModel(__argv[1]), b = MV1LoadModel(__argv[2]);
	if (a < 0 || b < 0) { fprintf(fp, "load fail\n"); fclose(fp); return 2; }
	const int headA = MV1SearchFrame(a, "head"), headB = MV1SearchFrame(b, "head");
	const int animNum = (std::min)(MV1GetAnimNum(a), MV1GetAnimNum(b));
	int siA = MakeARGB8ColorSoftImage(1280, 720), siB = MakeARGB8ColorSoftImage(1280, 720);
	struct Result { int big; int anim; float rate; int shot; long long headDiff; };
	std::vector<Result> results;
	double headMax = 0.0;
	for (int anim = 0; anim < animNum; ++anim)
	{
		for (float rate : kRates)
		{
			for (int s = 0; s < 3; ++s)
			{
				DrawPose(a, headA, anim, rate, kShots[s]);
				GetDrawScreenSoftImage(0, 0, 1280, 720, siA);
				const VECTOR pa = MV1GetFramePosition(a, headA);
				DrawPose(b, headB, anim, rate, kShots[s]);
				GetDrawScreenSoftImage(0, 0, 1280, 720, siB);
				const VECTOR pb = MV1GetFramePosition(b, headB);
				headMax = (std::max)(headMax, (double)VSize(VSub(pa, pb)));
				const unsigned char* pxA = static_cast<const unsigned char*>(GetImageAddressSoftImage(siA));
				const unsigned char* pxB = static_cast<const unsigned char*>(GetImageAddressSoftImage(siB));
				const int pitch = GetPitchSoftImage(siA);
				int big = 0;
				for (int y = 0; y < 720; ++y)
				{
					const unsigned char* ra = pxA + pitch * y;
					const unsigned char* rb = pxB + pitch * y;
					for (int x = 0; x < 1280; ++x)
					{
						int m = 0;
						for (int c = 0; c < 3; ++c) m = (std::max)(m, abs((int)ra[x * 4 + c] - (int)rb[x * 4 + c]));
						if (m > 32) ++big;
					}
				}
				results.push_back({ big, anim, rate, s, 0 });
			}
		}
	}
	std::sort(results.begin(), results.end(), [](const Result& x, const Result& y) { return x.big > y.big; });
	long long sum = 0;
	for (auto& r : results) sum += r.big;
	fprintf(fp, "anims=%d shots=%d  bone(head) max diff=%f  big-diff px: avg %.1f median %d worst %d\n", animNum, (int)results.size(), headMax, (double)sum / results.size(), results[results.size() / 2].big, results[0].big);
	for (int i = 0; i < 6; ++i)
	{
		const Result& r = results[i];
		fprintf(fp, "#%d big=%d anim=%d %s rate=%.2f %s\n", i, r.big, r.anim, MV1GetAnimName(a, r.anim), r.rate, kShots[r.shot].name);
		char name[64];
		DrawPose(a, headA, r.anim, r.rate, kShots[r.shot]);
		snprintf(name, sizeof(name), "_old%d.png", i);
		SaveDrawScreenToPNG(0, 0, 1280, 720, (std::string(__argv[4]) + name).c_str());
		DrawPose(b, headB, r.anim, r.rate, kShots[r.shot]);
		snprintf(name, sizeof(name), "_new%d.png", i);
		SaveDrawScreenToPNG(0, 0, 1280, 720, (std::string(__argv[4]) + name).c_str());
	}
	fclose(fp);
	DxLib_End();
	return 0;
}
