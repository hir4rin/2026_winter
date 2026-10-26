// 列車ステージとトンネル(とプレイヤー)を DxLib で描いて PNG に保存する(見た目の確認用)
// 使い方: preview.exe 出力.png カメラx y z 注視点x y z [敵の向き(度)]
// 敵は注視点の真下(y=0)、プレイヤーはその少し横に置く。標準ライトの値は preview_log.txt に書く
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(1280, 720, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	const char* data = "C:/Users/tsyts/GitHub/2026_winter/2026_winter/data/";
	char path[512];
	wsprintfA(path, "%sStage/TrainStage/TrainStage.mv1", data);
	int stage = MV1LoadModel(path);
	wsprintfA(path, "%sStage/TrainTunnel/TrainTunnel.mv1", data);
	int tunnel = MV1LoadModel(path);
	wsprintfA(path, "%sPlayer03.mv1", data);
	int player = MV1LoadModel(path);
	wsprintfA(path, "%sEnemy/Enemy.mv1", data);
	int enemy = MV1LoadModel(path);
	auto f = [](int i) { return static_cast<float>(atof(__argv[i])); };

	FILE* log = nullptr;
	fopen_s(&log, "preview_log.txt", "w");
	COLOR_F d = GetLightDifColor(), s = GetLightSpcColor(), a = GetLightAmbColor();
	VECTOR dir = GetLightDirection();
	fprintf(log, "standard light dif=(%.2f %.2f %.2f) spc=(%.2f %.2f %.2f) amb=(%.2f %.2f %.2f) dir=(%.2f %.2f %.2f)\n",
		d.r, d.g, d.b, s.r, s.g, s.b, a.r, a.g, a.b, dir.x, dir.y, dir.z);
	fclose(log);

	SetCameraNearFar(10.0f, 60000.0f);
	VECTOR cam = VGet(f(2), f(3), f(4)), target = VGet(f(5), f(6), f(7));
	SetCameraPositionAndTarget_UpVecY(cam, target);
	SetupCamera_Perspective(1.0f);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);

	auto addLight = [](VECTOR dir, float dif, float spc, float amb)
	{
		int h = CreateDirLightHandle(VNorm(dir));
		SetLightDifColorHandle(h, GetColorF(dif, dif, dif, 1.0f));
		SetLightSpcColorHandle(h, GetColorF(spc, spc, spc, 1.0f));
		SetLightAmbColorHandle(h, GetColorF(amb, amb, amb, 1.0f));
	};
	//ゲーム(SceneMain・Camera・TempTopLight)と同じライト。標準ライトは切る
	SetLightEnable(FALSE);
	addLight(VGet(0.0f, -1.0f, 1.0f), 0.6f, 0.3f, 0.3f);//カメラのライト(Camera::UpdateLight は呼ばれていないので向きは固定)
	addLight(VGet(-1.0f, -1.0f, 1.0f), 0.4f, 0.2f, 0.0f);
	addLight(VGet(0.0f, -1.0f, 0.0f), 0.5f, 0.0f, 0.25f + a.r);
	SetBackgroundColor(20, 20, 24);
	ClearDrawScreen();
	for (int i = -1; i <= 2; ++i)
	{
		MV1SetPosition(tunnel, VGet(0.0f, -150.0f, i * 13200.0f));
		MV1DrawModel(tunnel);
	}
	MV1DrawModel(stage);
	MV1SetPosition(player, VGet(target.x - 150.0f, 0.0f, target.z));
	MV1DrawModel(player);
	MV1SetPosition(enemy, VGet(target.x, 0.0f, target.z));
	MV1SetRotationXYZ(enemy, VGet(0.0f, __argc > 8 ? f(8) * DX_PI_F / 180.0f : 0.0f, 0.0f));
	MV1DrawModel(enemy);
	SaveDrawScreenToPNG(0, 0, 1280, 720, __argv[1]);
	DxLib_End();
	return 0;
}
