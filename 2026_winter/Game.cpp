#include "Game.h"

namespace
{
	int g_screenWidth = Game::kScreenWidth;//現在の実解像度(初期値は設計解像度)
	int g_screenHeight = Game::kScreenHeight;
}

namespace Game
{
	int GetScreenWidth()
	{
		return g_screenWidth;
	}

	int GetScreenHeight()
	{
		return g_screenHeight;
	}

	void SetScreenSize(int width, int height)
	{
		g_screenWidth = width;
		g_screenHeight = height;
	}

	float ScaleX(float designX)
	{
		return designX * g_screenWidth / static_cast<float>(kScreenWidth);
	}

	float ScaleY(float designY)
	{
		return designY * g_screenHeight / static_cast<float>(kScreenHeight);
	}

	float GetScale()
	{
		return static_cast<float>(g_screenWidth) / static_cast<float>(kScreenWidth);
	}
}
