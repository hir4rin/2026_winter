// 海賊の剣: .x → mv1、ボスの右手での持ち方(手のボーンからのずれ・回転)を計算して画像で確認する
#include "DxLib.h"
#include <cstdio>
#include <cmath>
#include <string>

static const char* kBoss = "C:/Users/tsyts/GitHub/2026_winter/2026_winter/data/Enemy/BossPirate/BossPirate.mv1";

static MATRIX Weapon(float px, float py, float pz, float rx, float ry, float rz, float s, MATRIX hand)
{
	MATRIX r = MMult(MMult(MGetRotX(rx), MGetRotY(ry)), MGetRotZ(rz));
	r = MMult(r, MGetTranslate(VGet(px, py, pz)));
	return MMult(MMult(MGetScale(VGet(s, s, s)), r), hand);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(1200, 600, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	FILE* f = nullptr; fopen_s(&f, "sword_log.txt", "w");

	int hx = MV1LoadModel("../sword/out/PirateSword.x");
	fprintf(f, "x handle=%d\n", hx);
	MV1SaveModelToMV1File(hx, "../sword/out/PirateSword.mv1");
	int sword = MV1LoadModel("../sword/out/PirateSword.mv1");
	fprintf(f, "semi after reload=%d texsemi=%d\n", MV1GetSemiTransState(sword), MV1GetTextureSemiTransState(sword, 0));
	VECTOR mn = MV1GetMeshMinPosition(sword, 0), mx = MV1GetMeshMaxPosition(sword, 0);
	fprintf(f, "sword mv1=%d min(%.1f %.1f %.1f) max(%.1f %.1f %.1f)\n", sword, mn.x, mn.y, mn.z, mx.x, mx.y, mx.z);

	int boss = MV1LoadModel(kBoss);
	int hand = MV1SearchFrame(boss, "hand_r");
	VECTOR handPos = MV1GetFramePosition(boss, hand);
	VECTOR lowerPos = MV1GetFramePosition(boss, MV1SearchFrame(boss, "lowerarm_r"));
	VECTOR middlePos = MV1GetFramePosition(boss, MV1SearchFrame(boss, "middle_01_r"));
	VECTOR footPos = MV1GetFramePosition(boss, MV1SearchFrame(boss, "foot_r"));
	VECTOR ballPos = MV1GetFramePosition(boss, MV1SearchFrame(boss, "ball_r"));
	VECTOR fwd = VSub(ballPos, footPos); fwd.y = 0; fwd = VNorm(fwd);
	VECTOR armDir = VNorm(VSub(handPos, lowerPos));
	fprintf(f, "fwd(%.2f %.2f %.2f) arm(%.2f %.2f %.2f)\n", fwd.x, fwd.y, fwd.z, armDir.x, armDir.y, armDir.z);

	//Tポーズでの剣の向き: 刃(+Y)=前、刃先側の護拳(-Z)=腕の向き
	VECTOR ay = fwd;
	VECTOR az = VScale(armDir, -1.0f);
	az = VNorm(VSub(az, VScale(ay, VDot(az, ay))));
	VECTOR ax = VCross(ay, az);
	//握りの位置: 手首と中指の付け根の間、少し下(手のひらは下向き)
	VECTOR grip = VAdd(VScale(VAdd(handPos, middlePos), 0.5f), VGet(0, -3.0f, 0));
	MATRIX sw = MGetIdent();
	sw.m[0][0] = ax.x; sw.m[0][1] = ax.y; sw.m[0][2] = ax.z;
	sw.m[1][0] = ay.x; sw.m[1][1] = ay.y; sw.m[1][2] = ay.z;
	sw.m[2][0] = az.x; sw.m[2][1] = az.y; sw.m[2][2] = az.z;
	sw.m[3][0] = grip.x; sw.m[3][1] = grip.y; sw.m[3][2] = grip.z;

	MATRIX handWorld = MV1GetFrameLocalWorldMatrix(boss, hand);
	MATRIX rel = MMult(sw, MInverse(handWorld));
	//rel = R(XYZ) * T//R = Rx*Ry*Rz(行ベクトル)
	float b = asinf(-rel.m[0][2]);
	float c = atan2f(rel.m[0][1], rel.m[0][0]);
	float a = atan2f(rel.m[1][2], rel.m[2][2]);
	const float toDeg = 180.0f / DX_PI_F;
	fprintf(f, "OFFSET %.2f %.2f %.2f ROT %.2f %.2f %.2f\n", rel.m[3][0], rel.m[3][1], rel.m[3][2], a * toDeg, b * toDeg, c * toDeg);
	MATRIX check = Weapon(rel.m[3][0], rel.m[3][1], rel.m[3][2], a, b, c, 1.0f, handWorld);
	float err = 0; for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) err += fabsf(check.m[i][j] - sw.m[i][j]);
	fprintf(f, "reconstruct err=%.4f\n", err);

	struct Shot { const char* anim; float time; };
	Shot shots[] = { {"", 0}, {"Idle", 60}, {"Sword And Shield Attack", 30}, {"Combo_Attack_04_01", 20}, {"Block_Loop", 20}, {"Sword And Shield Run", 8} };
	int no = 0;
	for (auto& s : shots)
	{
		int at = -1;
		if (s.anim[0]) { at = MV1AttachAnim(boss, MV1GetAnimIndex(boss, s.anim), -1, FALSE); MV1SetAttachAnimTime(boss, at, s.time); }
		ClearDrawScreen();
		DrawBox(0, 0, 1200, 600, GetColor(90, 90, 100), TRUE);
		SetCameraNearFar(10.0f, 3000.0f);
		for (int view = 0; view < 2; ++view)
		{
			SetDrawArea(view * 600, 0, view * 600 + 600, 600);
			SetCameraScreenCenter((float)(view * 600 + 300), 300.0f);
			VECTOR camPos = view == 0 ? VAdd(VGet(0, 120, 0), VScale(fwd, 380.0f)) : VGet(-380, 120, 0);
			SetCameraPositionAndTarget_UpVecY(camPos, VGet(0, 110, 0));
			MV1DrawModel(boss);
			MV1SetMatrix(sword, Weapon(rel.m[3][0], rel.m[3][1], rel.m[3][2], a, b, c, 1.0f, MV1GetFrameLocalWorldMatrix(boss, hand)));
			MV1DrawModel(sword);
		}
		SetDrawArea(0, 0, 1200, 600);
		DrawString(10, 10, s.anim[0] ? s.anim : "T-pose", GetColor(255, 255, 255));
		SaveDrawScreenToPNG(0, 0, 1200, 600, ("sword_" + std::to_string(no++) + ".png").c_str());
		if (at >= 0) MV1DetachAnim(boss, at);
	}
	fclose(f);
	DxLib_End();
	return 0;
}
