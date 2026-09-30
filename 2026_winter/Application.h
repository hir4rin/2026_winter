#pragma once

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

