// ゲームのウィンドウの範囲が画面上で実際に書き換わった時刻を Desktop Duplication で記録する
// 使い方: dupmon.exe 秒数 出力.txt [出力の番号]
//   2026_winter.exe のウィンドウを探し、そのウィンドウと重なる「変わった範囲」がある更新だけを数える
//   新しい絵が来た間隔を全部書き、最後に「25ms 以上空いた回数」などを出す
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <psapi.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "user32.lib")

static HWND g_game = nullptr;
static BOOL CALLBACK FindGame(HWND h, LPARAM)
{
	if (!IsWindowVisible(h)) return TRUE;
	DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
	HANDLE p = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
	if (!p) return TRUE;
	char name[MAX_PATH] = {}; DWORD n = MAX_PATH;
	QueryFullProcessImageNameA(p, 0, name, &n);
	CloseHandle(p);
	if (strstr(name, "2026_winter.exe")) { g_game = h; return FALSE; }
	return TRUE;
}

int main(int argc, char** argv)
{
	double seconds = argc > 1 ? atof(argv[1]) : 30.0;
	const char* out = argc > 2 ? argv[2] : "dupmon.txt";
	int want = argc > 3 ? atoi(argv[3]) : 0;
	for (int i = 0; i < 100 && !g_game; ++i) { EnumWindows(FindGame, 0); if (!g_game) Sleep(100); }
	if (!g_game) { printf("game window not found\n"); return 1; }
	RECT wr; GetClientRect(g_game, &wr);
	POINT tl = { wr.left, wr.top }, br = { wr.right, wr.bottom };
	ClientToScreen(g_game, &tl); ClientToScreen(g_game, &br);

	ID3D11Device* dev = nullptr; ID3D11DeviceContext* ctx = nullptr;
	D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx);
	IDXGIDevice* dxdev = nullptr; dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxdev);
	IDXGIAdapter* ad = nullptr; dxdev->GetAdapter(&ad);
	IDXGIOutput* o = nullptr; ad->EnumOutputs(want, &o);
	DXGI_OUTPUT_DESC od; o->GetDesc(&od);
	// 出力の座標に直したゲームの範囲
	RECT game = { tl.x - od.DesktopCoordinates.left, tl.y - od.DesktopCoordinates.top, br.x - od.DesktopCoordinates.left, br.y - od.DesktopCoordinates.top };
	DEVMODEW dm = {}; dm.dmSize = sizeof(dm); EnumDisplaySettingsW(od.DeviceName, ENUM_CURRENT_SETTINGS, &dm);
	printf("output %ls %luHz game rect (%ld,%ld)-(%ld,%ld)\n", od.DeviceName, dm.dmDisplayFrequency, game.left, game.top, game.right, game.bottom);
	IDXGIOutput1* o1 = nullptr; o->QueryInterface(__uuidof(IDXGIOutput1), (void**)&o1);
	IDXGIOutputDuplication* dup = nullptr;
	if (FAILED(o1->DuplicateOutput(dev, &dup))) { printf("DuplicateOutput failed\n"); return 1; }
	LARGE_INTEGER freq, start; QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&start);
	std::vector<double> times;
	std::vector<BYTE> meta;
	for (;;)
	{
		LARGE_INTEGER now; QueryPerformanceCounter(&now);
		if ((now.QuadPart - start.QuadPart) / (double)freq.QuadPart > seconds) break;
		DXGI_OUTDUPL_FRAME_INFO info; IDXGIResource* res = nullptr;
		HRESULT hr = dup->AcquireNextFrame(100, &info, &res);
		if (hr == DXGI_ERROR_WAIT_TIMEOUT) continue;
		if (FAILED(hr)) break;
		bool hitGame = false;
		if (info.LastPresentTime.QuadPart != 0 && info.TotalMetadataBufferSize > 0)
		{
			meta.resize(info.TotalMetadataBufferSize);
			UINT used = 0;
			if (SUCCEEDED(dup->GetFrameDirtyRects((UINT)meta.size(), (RECT*)meta.data(), &used)))
			{
				RECT* r = (RECT*)meta.data();
				for (UINT i = 0; i < used / sizeof(RECT); ++i)
				{
					RECT x;
					if (IntersectRect(&x, &r[i], &game) && (x.right - x.left) * (x.bottom - x.top) > 10000) { hitGame = true; break; }
				}
			}
		}
		if (hitGame) times.push_back(info.LastPresentTime.QuadPart * 1000.0 / freq.QuadPart);
		if (res) res->Release();
		dup->ReleaseFrame();
	}
	FILE* f = nullptr; fopen_s(&f, out, "w");
	int gap25 = 0, gap40 = 0, n = 0; double sum = 0, mx = 0;
	for (size_t i = 1; i < times.size(); ++i)
	{
		double d = times[i] - times[i - 1];
		fprintf(f, "%.2f\n", d);
		++n; sum += d; if (d > mx) mx = d;
		if (d > 25.0) ++gap25;
		if (d > 40.0) ++gap40;
	}
	fprintf(f, "# frames %d avg %.2f max %.2f gap>25ms %d gap>40ms %d\n", n, n ? sum / n : 0, mx, gap25, gap40);
	printf("frames %d avg %.2f max %.2f gap>25ms %d gap>40ms %d\n", n, n ? sum / n : 0, mx, gap25, gap40);
	fclose(f);
	return 0;
}
