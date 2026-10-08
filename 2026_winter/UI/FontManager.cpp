#include "FontManager.h"
#include "DxLib.h"

#include <cassert>

FontManager* FontManager::m_instance = nullptr;

namespace
{
	const char* const kFontPath = "Data/Font/SourceHanSerifJP-ExtraLight.otf";
	const char* const kFontName = "源ノ明朝 JP ExtraLight";

	const int kFontSize[] =
	{
		8,16,24,32,40,48,56,64,72,80,88
	};
}

FontManager::~FontManager()
{
	for (const auto& f : m_fonts)
	{
		DeleteFontToHandle(f.handle);
	}
	m_fonts.clear();
}

void FontManager::LoadFont()
{
	//読み込むフォントファイルのパス
	if (AddFontResourceEx(kFontPath, FR_PRIVATE, NULL) > 0)
	{
	}
	else
	{
		//フォント読込エラー処理
		MessageBox(NULL, "フォント読込失敗", "", MB_OK);
	}

	//とりあえず使いそうなサイズのフォントを作っておく
	for (auto& size : kFontSize)
	{
		Font add;
		add.fontName = kFontName;
		add.size = size;
		add.handle = CreateFontToHandle(kFontName, size, -1, DX_FONTTYPE_EDGE, -1, 1);
		m_fonts.push_back(add);
	}
}

void FontManager::DrawCenteredText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor)
{
	Font drawFont;
	bool isFound = false;

	for (auto& font : m_fonts)
	{
		if (/*font.fontName == fontName &&*/ font.size == size)
		{
			isFound = true;
			drawFont = font;
		}
	}

	if (!isFound)
	{
#ifdef _DEBUG
		assert(0 && "フォントデータが見つかりませんでした");
#endif
		return;
	}

	int textWidth = GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.length()), drawFont.handle);
	int textHeight = GetFontSizeToHandle(drawFont.handle);
	DrawStringToHandle(x - textWidth / 2, y - textHeight / 2, text.c_str(), color, drawFont.handle, edgeColor);
}

void FontManager::DrawBottomRightText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor)
{
	Font drawFont;
	bool isFound = false;

	for (auto& font : m_fonts)
	{
		if (/*font.fontName == fontName && */font.size == size)
		{
			isFound = true;
			drawFont = font;
		}
	}

	if (!isFound)
	{
#ifdef _DEBUG
		assert(0 && "フォントデータが見つかりませんでした");
#endif
		return;
	}

	int textWidth = GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.length()), drawFont.handle);
	int textHeight = GetFontSizeToHandle(drawFont.handle);
	DrawStringToHandle(x - textWidth, y - textHeight, text.c_str(), color, drawFont.handle, edgeColor);
}

void FontManager::DrawLeftText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor)
{
	Font drawFont;
	bool isFound = false;

	for (auto& font : m_fonts)
	{
		if (/*font.fontName == fontName && */font.size == size)
		{
			isFound = true;
			drawFont = font;
		}
	}

	if (!isFound)
	{
#ifdef _DEBUG
		assert(0 && "フォントデータが見つかりませんでした");
#endif
		return;
	}

	int textHeight = GetFontSizeToHandle(drawFont.handle);
	//左端揃えの文字列描画
	DrawStringToHandle(x, y - textHeight / 2, text.c_str(), color, drawFont.handle,edgeColor);
}

void FontManager::DrawCenteredExtendText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor, float extendRate)
{
	Font drawFont;
	bool isFound = false;

	for (auto& font : m_fonts)
	{
		if (/*font.fontName == fontName &&*/ font.size == size)
		{
			isFound = true;
			drawFont = font;
		}
	}

	if (!isFound)
	{
#ifdef _DEBUG
		assert(0 && "フォントデータが見つかりませんでした");
#endif
		return;
	}

	int textWidth = static_cast<int>(GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.length()), drawFont.handle) * extendRate);
	int textHeight = static_cast<int>(GetFontSizeToHandle(drawFont.handle) * extendRate);
	DrawExtendStringToHandle(x - textWidth / 2, y - textHeight / 2, extendRate, extendRate, text.c_str(), color, drawFont.handle, edgeColor);
}

void FontManager::DrawBottomRightAndQuakeText(int x, int y, std::string text, unsigned int color, int size, unsigned int edgeColor, bool isShake, int shakeAmplitude, int shakeSpeed)
{
	Font drawFont;
	bool isFound = false;

	for (auto& font : m_fonts)
	{
		if (/*font.fontName == fontName && */font.size == size)
		{
			isFound = true;
			drawFont = font;
		}
	}

	if (!isFound)
	{
#ifdef _DEBUG
		assert(0 && "フォントデータが見つかりませんでした");
#endif
		return;
	}

	if (isShake)
	{
		//現在のフレーム数を取得
		int frameCount = GetNowCount() / shakeSpeed / 5;
		//揺れの計算(sinを使用して滑らかに左右に動く)
		int shakeOffset = static_cast<int>(sin(frameCount) * shakeAmplitude / 6);

		x += shakeOffset;

		//int sub = 0xff0000 - color;
		/*float per = static_cast<float>(shakeAmplitude) / 30.0f;
		color += static_cast<unsigned int>(sub * per);*/
	}

	int textWidth = GetDrawStringWidthToHandle(text.c_str(), static_cast<int>(text.length()), drawFont.handle);
	int textHeight = GetFontSizeToHandle(drawFont.handle);
	DrawStringToHandle(x - textWidth, y - textHeight, text.c_str(), color, drawFont.handle, edgeColor);
}
