// 列車ステージに扉(ゲート)を置いて PNG に保存する(開き方の確認用)
// 使い方: preview_gate.exe 出力.png カメラx y z 注視点x y z 扉のz 開き具合(0〜1) 横の縮み(開いたときのXの倍率) 横の移動(開いたとき)
#include "DxLib.h"
#include <cstdlib>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	ChangeWindowMode(TRUE); SetGraphMode(1280, 720, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	auto f = [](int i) { return static_cast<float>(atof(__argv[i])); };
	const char* data = "C:/Users/tsyts/GitHub/2026_winter/2026_winter/data/";
	char path[512];
	wsprintfA(path, "%sStage/TrainStage/TrainStage.mv1", data);
	int stage = MV1LoadModel(path);
	wsprintfA(path, "%sStage/TrainTunnel/TrainTunnel.mv1", data);
	int tunnel = MV1LoadModel(path);
	wsprintfA(path, "%sStage/Gates/CorridorDoorL.mv1", data);
	int doorL = MV1LoadModel(path);
	wsprintfA(path, "%sStage/Gates/CorridorDoorR.mv1", data);
	int doorR = MV1LoadModel(path);
	wsprintfA(path, "%sPlayer03.mv1", data);
	int player = MV1LoadModel(path);

	SetCameraNearFar(10.0f, 60000.0f);
	VECTOR target = VGet(f(5), f(6), f(7));
	SetCameraPositionAndTarget_UpVecY(VGet(f(2), f(3), f(4)), target);
	SetupCamera_Perspective(1.0f);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	auto addLight = [](VECTOR dir, float dif, float spc, float amb)
	{
		int h = CreateDirLightHandle(VNorm(dir));
		SetLightDifColorHandle(h, GetColorF(dif, dif, dif, 1.0f));
		SetLightSpcColorHandle(h, GetColorF(spc, spc, spc, 1.0f));
		SetLightAmbColorHandle(h, GetColorF(amb, amb, amb, 1.0f));
	};
	COLOR_F a = GetLightAmbColor();
	SetLightEnable(FALSE);
	addLight(VGet(0.0f, -1.0f, 1.0f), 0.6f, 0.3f, 0.3f);
	addLight(VGet(-1.0f, -1.0f, 1.0f), 0.4f, 0.2f, 0.0f);
	addLight(VGet(0.0f, -1.0f, 0.0f), 0.5f, 0.0f, 0.25f + a.r);
	SetBackgroundColor(20, 20, 24);
	ClearDrawScreen();
	for (int i = -1; i <= 2; ++i) { MV1SetPosition(tunnel, VGet(0.0f, -150.0f, i * 13200.0f)); MV1DrawModel(tunnel); }
	MV1DrawModel(stage);
	const float z = f(8), t = f(9), sx = 1.0f + (f(10) - 1.0f) * t, mx = f(11) * t;
	MV1SetMatrix(doorL, MMult(MGetScale(VGet(sx, 1, 1)), MGetTranslate(VGet(-699.6f - mx, 2.6f, z))));
	MV1SetMatrix(doorR, MMult(MGetScale(VGet(sx, 1, 1)), MGetTranslate(VGet(708.4f + mx, 2.6f, z))));
	MV1DrawModel(doorL); MV1DrawModel(doorR);
	MV1SetPosition(player, VGet(target.x - 150.0f, 0.0f, target.z - 600.0f));
	MV1DrawModel(player);
	SaveDrawScreenToPNG(0, 0, 1280, 720, __argv[1]);
	DxLib_End();
	return 0;
}
