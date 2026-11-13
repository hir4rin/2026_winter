#include "UIFont.h"
#include "DxLib.h"

namespace
{
	const char* const kFontName = "游明朝";
}

int UIFont::Create(int size, int thick, int fontType, int edgeSize)
{
	return CreateFontToHandle(GetName(), size, thick, fontType, -1, edgeSize);
}

const char* UIFont::GetName()
{
	return kFontName;
}
