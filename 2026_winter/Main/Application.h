#pragma once

struct IDXGIOutput;

struct Size
{
	int w;
	int h;
};

class Application
{
private:
	Size m_windowSize;

	Application();
	Application(const Application& app) = delete;//コピーコンストラクタでの複製を防ぐ
	//
	void operator=(const Application& app) = delete;

	bool m_requestedExit = false;//ゲームの終了をがリクエスト
	bool m_isFullScreen = false;//現在フルスクリーンかどうか
	bool m_wasF2Pressed = false;//F2キーの押しっぱなし判定用
	bool m_isVSyncPacing = false;//モニターが60Hz前後か
	//60fps の合わせ方
	enum class PaceMode
	{
		Busy,//自前の待ちだけ
		VSyncAndBusy,//ScreenFlip の VSync 待ち＋自前の待ち(前のやり方)
		WaitVSync,//WaitVSync でモニターの更新を待ってから出す
		Dwm,//出した後に DwmFlush で Windows の画面の合成を待つ(ウィンドウモード用)
		VBlank,//DXGI の WaitForVBlank でモニターの更新を待ってから出す(WaitVSync はたまに更新を見逃すため)
	};
	PaceMode m_paceMode = PaceMode::Busy;

	/// <summary>60fps の合わせ方(m_paceMode)を決める。DxLib_Init の後に呼ぶ</summary>
	void InitFramePacing();

	IDXGIOutput* m_vblankOutput = nullptr;//ウィンドウがあるモニター(WaitForVBlank 用)
	void* m_vblankMonitor = nullptr;//m_vblankOutput のモニター(HMONITOR。ウィンドウが別のモニターに移ったら取り直す)
	/// <summary>モニターの更新を待つ。待てなかったら false</summary>
	bool WaitVBlank();
	/// <summary>ウィンドウがあるモニターの IDXGIOutput を取り直す</summary>
	void UpdateVBlankOutput();
	void ReleaseVBlankOutput();
	/// <summary>今がモニターの何回目の更新か(取れなければ -1。FrameProfiler でカクッを数える用)</summary>
	long long GetRefreshIndex()const;

public:
	~Application();
	static Application& GetInstance()
	{
		// staticなのでメモリの場所が一つで確定→これ一つしか実態がない→シングルトンクラス
		static Application instance;
		return instance;	// インスタンスの参照を返す→Applicationクラスの場所を返す
		// staticで一つしか存在しないものの参照を返すので複数になることはない
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	/// <returns>初期化成功:true / 初期化失敗:false</returns>
	bool Init();

	/// <summary>
	/// アプリケーションをスタートする
	/// ゲームループを終了する
	/// </summary>
	void Run();
	/// <summary>
	/// アプリケーションの終了
	/// </summary>
	void Terminate();


};

