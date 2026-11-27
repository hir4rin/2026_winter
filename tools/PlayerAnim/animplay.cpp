// アニメのmv1を、プレイヤーに刀を持たせてループ再生して見る(横・前・斜めの3つ)
// animplay.exe Player03.mv1 アニメのmv1 刀のmv1 [ウィンドウのX Y]
// 環境変数 ANIMPLAY_BAZ=RPG7.mv1 で、名前に Bazooka が入ったアニメは左手にバズーカ。ANIMPLAY_ANIM=最初のアニメの番号
// ←→: アニメを切り替え / Space: 止める / ,.: 止めて1コマ送り / ↑↓: 速さ / Esc: 終わる
#include "DxLib.h"
#include <cstdlib>
#include <cmath>
#include <cstring>

static MATRIX WeaponMat(int model, int hand)
{
	MATRIX rot = MGetRotX(90.0f * DX_PI_F / 180.0f);
	rot = MMult(rot, MGetRotY(90.0f * DX_PI_F / 180.0f + DX_PI_F / 2));
	rot = MMult(rot, MGetRotZ(0.0f - DX_PI_F / 2));
	return MMult(MMult(MGetScale(VGet(0.6f, 0.6f, 0.6f)), rot), MV1GetFrameLocalWorldMatrix(model, hand));
}

// バズーカ: 肩の上から手の上への向き、原点(引き金)は左手(bazshot と同じ)
static MATRIX BazookaMat(int model, int hand, int upperArm, int spine, int neck)
{
	const VECTOR up0 = VNorm(VSub(MV1GetFramePosition(model, neck), MV1GetFramePosition(model, spine)));
	const VECTOR grip = MV1GetFramePosition(model, hand);
	const VECTOR back = VAdd(MV1GetFramePosition(model, upperArm), VScale(up0, 9.0f));
	const VECTOR z = VNorm(VSub(VAdd(grip, VScale(up0, 9.0f)), back));
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
	const int SW = 1500, SH = 640, VW = SW / 3;
	ChangeWindowMode(TRUE); SetGraphMode(SW, SH, 32); SetOutApplicationLogValidFlag(FALSE);
	SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);
	SetMainWindowText("animplay");
	SetWindowInitPosition((__argc > 5) ? atoi(__argv[4]) : -1700, (__argc > 5) ? atoi(__argv[5]) : 120);
	SetAlwaysRunFlag(TRUE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	const int player = MV1LoadModel(__argv[1]);
	const int src = MV1LoadModel(__argv[2]);
	const int sword = MV1LoadModel(__argv[3]);
	const int hand = MV1SearchFrame(player, "hand_r");
	const int pelvis = MV1SearchFrame(player, "pelvis");
	const int animNum = MV1GetAnimNum(src);
	//環境変数 ANIMPLAY_BAZ = RPG7.mv1 なら、名前に Bazooka が入ったアニメで左手にバズーカ。ANIMPLAY_ANIM = 最初のアニメの番号
	const int baz = getenv("ANIMPLAY_BAZ") ? MV1LoadModel(getenv("ANIMPLAY_BAZ")) : -1;
	const int handL = MV1SearchFrame(player, "hand_l"), upperL = MV1SearchFrame(player, "upperarm_l");
	const int spine = MV1SearchFrame(player, "spine_03"), neck = MV1SearchFrame(player, "neck_01");
	int anim = getenv("ANIMPLAY_ANIM") ? atoi(getenv("ANIMPLAY_ANIM")) % animNum : 0;
	int at = MV1AttachAnim(player, anim, src, FALSE);
	float t = 0.0f, speed = 1.0f; int wait = 0; bool pause = false;
	int prevKey[256] = {}; char key[256];
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	SetGlobalAmbientLight(GetColorF(0.35f, 0.35f, 0.35f, 1.0f));
	while (ProcessMessage() == 0)
	{
		GetHitKeyStateAll(key);
		auto trig = [&](int k) { return key[k] && !prevKey[k]; };
		if (key[KEY_INPUT_ESCAPE]) break;
		if (trig(KEY_INPUT_RIGHT) || trig(KEY_INPUT_LEFT))
		{
			anim = (anim + (key[KEY_INPUT_RIGHT] ? 1 : animNum - 1)) % animNum;
			MV1DetachAnim(player, at); at = MV1AttachAnim(player, anim, src, FALSE); t = 0.0f; wait = 0;
		}
		if (trig(KEY_INPUT_SPACE)) pause = !pause;
		if (trig(KEY_INPUT_UP)) speed = (speed >= 1.0f) ? 1.0f : speed * 2.0f;
		if (trig(KEY_INPUT_DOWN)) speed = (speed <= 0.125f) ? 0.125f : speed * 0.5f;
		const float len = MV1GetAnimTotalTime(src, anim);
		if (trig(KEY_INPUT_PERIOD)) { pause = true; t = (t + 1.0f > len) ? 0.0f : floorf(t + 1.0f); }
		if (trig(KEY_INPUT_COMMA)) { pause = true; t = (t - 1.0f < 0.0f) ? len : ceilf(t - 1.0f); }
		for (int i = 0; i < 256; ++i) prevKey[i] = key[i];
		// 30fps のアニメを 60fps で回す。最後で少し止めてから頭へ
		if (!pause)
		{
			if (wait > 0) { if (--wait == 0) t = 0.0f; }
			else { t += 0.5f * speed; if (t >= len) { t = len; wait = 30; } }
		}
		MV1SetAttachAnimTime(player, at, t);

		SetDrawArea(0, 0, SW, SH);
		ClearDrawScreen();
		DrawBox(0, 0, SW, SH, GetColor(60, 60, 75), TRUE);
		const VECTOR hips = MV1GetFramePosition(player, pelvis);
		const VECTOR dirs[3] = { VGet(1.0f, 0.05f, 0.0f), VGet(0.0f, 0.1f, -1.0f), VGet(0.8f, 0.45f, -1.0f) };
		const char* names[3] = { "横", "前", "斜め上" };
		for (int v = 0; v < 3; ++v)
		{
			const int x0 = VW * v;
			SetDrawArea(x0, 0, x0 + VW, SH);
			SetCameraScreenCenter(x0 + VW * 0.5f, SH * 0.5f);
			SetupCamera_Perspective(1.0f);
			SetCameraNearFar(10, 5000);
			const VECTOR target = VGet(0.0f, 90.0f, 0.0f);
			SetCameraPositionAndTarget_UpVecY(VAdd(target, VScale(VNorm(dirs[v]), 420.0f)), target);
			ClearDrawScreenZBuffer();
			for (int g = -4; g <= 4; ++g)//床
			{
				DrawLine3D(VGet(g * 50.0f, 0, -200), VGet(g * 50.0f, 0, 200), GetColor(90, 90, 110));
				DrawLine3D(VGet(-200, 0, g * 50.0f), VGet(200, 0, g * 50.0f), GetColor(90, 90, 110));
			}
			MV1DrawModel(player);
			MV1SetMatrix(sword, WeaponMat(player, hand));
			MV1DrawModel(sword);
			if (baz >= 0 && strstr(MV1GetAnimName(src, anim), "Bazooka"))
			{
				MV1SetMatrix(baz, BazookaMat(player, handL, upperL, spine, neck));
				MV1DrawModel(baz);
			}
			DrawLine(x0, 0, x0, SH, GetColor(30, 30, 40));
			DrawString(x0 + 8, SH - 24, names[v], GetColor(200, 200, 200));
		}
		SetDrawArea(0, 0, SW, SH);
		DrawFormatString(8, 8, GetColor(255, 255, 0), "%s  %d / %d F  x%.3g%s", MV1GetAnimName(src, anim), (int)t, (int)len, speed, pause ? "  (止め)" : "");
		DrawString(8, 30, "←→ 切り替え  Space 止める  , . コマ送り  ↑↓ 速さ  Esc 終わる", GetColor(180, 180, 180));
		(void)hips;
		//確認用: 環境変数 ANIMPLAY_SNAP があれば 20 コマ目を保存
		static int frameCount = 0;
		if (++frameCount == 20 && getenv("ANIMPLAY_SNAP")) SaveDrawScreenToPNG(0, 0, SW, SH, getenv("ANIMPLAY_SNAP"));
		ScreenFlip();
	}
	DxLib_End();
	return 0;
}
