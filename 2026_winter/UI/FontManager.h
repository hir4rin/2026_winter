#pragma once
#include <string>
#include <list>

class FontManager
{
private:
	struct Font
	{
		std::string fontName;
		int size;

		int handle;
	};
private:
	//コンストラクタ
	FontManager() {};
	//デストラクタ
	virtual ~FontManager();
	//インスタンス
	static FontManager* m_instance;

public:
	//コピーコンストラクタから実体の生成ができてしまうため
	//コピーコンストラクタを禁止する
	FontManager(const FontManager&) = delete;
	FontManager& operator=(const FontManager&) = delete;
	FontManager(FontManager&&) = delete;
	FontManager& operator= (const FontManager&&) = delete;

	static FontManager& Instance()
	{
		if (!m_instance)
		{
			m_instance = new FontManager;
		}

		return *m_instance;
	}
	//メモリリーク対策
	//削除
	static void Destroy()
	{
		delete m_instance;
		m_instance = nullptr;
	}

public:
	void LoadFont();

	// 指定座標を中心とする文字列を描画
	//const int kFontSize[] ={	8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88};
	void DrawCenteredText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor);
	// 指定座標を右下とする文字列を描画
	//const int kFontSize[] ={	8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88};
	void DrawBottomRightText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor);
	// 指定座標を左端とする文字列を描画
	//const int kFontSize[] ={	8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88};
	void DrawLeftText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor);


	void DrawCenteredExtendText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor, float extendRate);
	/// <summary>
	/// 指定座標を右下とする文字列を描画し、フラグがtrueになると文字を揺らす
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="text"></param>
	/// <param name="color"></param>
	/// <param name="size"></param>
	/// <param name="isShake"></param>
	/// <param name="shakeAmplitude">振れ幅</param>
	/// <param name="shakeSpeed">揺れの速さ</param>
	void DrawBottomRightAndQuakeText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor, bool isShake = false, int shakeAmplitude = 5, int shakeSpeed = 2);

public:
	std::list<Font> m_fonts;
};