#include "DxLib.h"
#include "Game.h"
#include "Application.h"

#include <memory> //シェアードポインタ



// プログラムは WinMain から始まります
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	auto& app = Application::GetInstance();
	if (!app.Init())
	{
		return -1;
	}
	app.Run();

	app.Terminate();

	return 0;				// ソフトの終了
}
