// ステージの当たり判定を真上から描いて PNG にする(扉の場所を決める用)
// 使い方: map_stage.exe 当たり判定.mv1 出力.png 中心x 中心z 幅(cm) [CSVの点 ...]
//   床は高さで色分け(低い=青〜高い=赤)、壁(法線が横)は白。CSV(敵配置・エリア)は x,z を点と名前で描く
#include "DxLib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	const int W = 1400, H = 1400;
	ChangeWindowMode(TRUE); SetGraphMode(W, H, 32); SetWindowVisibleFlag(FALSE); SetOutApplicationLogValidFlag(FALSE);
	if (DxLib_Init() == -1) return 1;
	SetDrawScreen(DX_SCREEN_BACK);
	auto f = [](int i) { return (float)atof(__argv[i]); };
	int h = MV1LoadModel(__argv[1]);
	const float cx = f(3), cz = f(4), size = f(5);
	SetUseZBuffer3D(TRUE); SetWriteZBuffer3D(TRUE);
	SetCameraNearFar(10.0f, 200000.0f);
	SetCameraPositionAndTargetAndUpVec(VGet(cx, 50000.0f, cz), VGet(cx, 0.0f, cz), VGet(0, 0, 1));
	SetupCamera_Ortho(size);
	SetBackgroundColor(10, 10, 10);
	ClearDrawScreen();
	MV1SetupReferenceMesh(h, -1, TRUE);
	MV1_REF_POLYGONLIST r = MV1GetReferenceMesh(h, -1, TRUE);
	for (int i = 0; i < r.PolygonNum; ++i)
	{
		VECTOR p[3];
		for (int k = 0; k < 3; ++k) p[k] = r.Vertexs[r.Polygons[i].VIndex[k]].Position;
		VECTOR n = VNorm(VCross(VSub(p[1], p[0]), VSub(p[2], p[0])));
		unsigned int col;
		if (fabsf(n.y) < 0.5f) col = GetColor(255, 255, 255);
		else
		{
			float y = (p[0].y + p[1].y + p[2].y) / 3.0f;
			float t = (y + 800.0f) / 2000.0f; t = t < 0 ? 0 : (t > 1 ? 1 : t);
			col = GetColor((int)(60 + 180 * t), 60, (int)(220 - 180 * t));
		}
		DrawTriangle3D(p[0], p[1], p[2], col, TRUE);
	}
	SetUseZBuffer3D(FALSE);
	//格子(10mごと)
	const int step = 1000;
	for (int gx = (int)((cx - size) / step) * step; gx < cx + size; gx += step)
	{
		VECTOR a = ConvWorldPosToScreenPos(VGet((float)gx, 0, cz - size)), b = ConvWorldPosToScreenPos(VGet((float)gx, 0, cz + size));
		DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, GetColor(40, 90, 40));
		VECTOR t = ConvWorldPosToScreenPos(VGet((float)gx, 0, cz));
		DrawFormatString((int)t.x + 2, 2, GetColor(80, 200, 80), "%d", gx);
	}
	for (int gz = (int)((cz - size) / step) * step; gz < cz + size; gz += step)
	{
		VECTOR a = ConvWorldPosToScreenPos(VGet(cx - size, 0, (float)gz)), b = ConvWorldPosToScreenPos(VGet(cx + size, 0, (float)gz));
		DrawLine((int)a.x, (int)a.y, (int)b.x, (int)b.y, GetColor(40, 90, 40));
		VECTOR t = ConvWorldPosToScreenPos(VGet(cx, 0, (float)gz));
		DrawFormatString(2, (int)t.y + 2, GetColor(80, 200, 80), "%d", gz);
	}
	//CSV の点
	for (int a = 6; a < __argc; ++a)
	{
		std::ifstream file(__argv[a]);
		std::string line;
		bool isArea = strstr(__argv[a], "PhaseAreas") != nullptr;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#') continue;
			std::vector<std::string> t; std::stringstream ss(line); std::string s;
			while (std::getline(ss, s, ',')) t.push_back(s);
			if (t.size() < 6) continue;
			float x = (float)atof(t[1].c_str()), z = (float)atof(t[3].c_str());
			VECTOR sp = ConvWorldPosToScreenPos(VGet(x, 0, z));
			if (isArea)
			{
				float hx = (float)atof(t[4].c_str()), hz = (float)atof(t[6].c_str());
				VECTOR a0 = ConvWorldPosToScreenPos(VGet(x - hx, 0, z - hz)), a1 = ConvWorldPosToScreenPos(VGet(x + hx, 0, z + hz));
				DrawBox((int)a0.x, (int)a1.y, (int)a1.x, (int)a0.y, GetColor(255, 255, 0), FALSE);
				DrawString((int)sp.x + 4, (int)sp.y, t[0].c_str(), GetColor(255, 255, 0));
			}
			else
			{
				DrawCircle((int)sp.x, (int)sp.y, 4, GetColor(255, 80, 80), TRUE);
				DrawFormatString((int)sp.x + 5, (int)sp.y - 6, GetColor(255, 160, 160), "%s%s", t[7].c_str(), t[5].c_str());
			}
		}
	}
	SaveDrawScreenToPNG(0, 0, W, H, __argv[2]);
	DxLib_End();
	return 0;
}
