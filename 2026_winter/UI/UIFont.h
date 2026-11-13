#pragma once

//ゲームのUIの文字のフォント(フォントを変えるときはここだけ直す)
class UIFont
{
public:
	//CreateFontToHandle と同じ引数(フォント名だけ決めてくれる)
	static int Create(int size, int thick, int fontType, int edgeSize = -1);
	static const char* GetName();
};
