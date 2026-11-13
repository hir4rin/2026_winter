#pragma once
#include <string>

//Tutorial.csv の1行(1ページ)
struct TutorialPageData
{
	std::string id;
	int page = 0;
	std::string type;//Image / Movie
	std::string file;//画像
	std::string title;//タイトル文字
	std::string text;//中のテキスト
};
