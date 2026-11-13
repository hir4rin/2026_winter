#include "MenuUI.h"
#include <cmath>
#include <algorithm>
#include "DxLib.h"
#include "Main/Game.h"
#include "UIFont.h"

namespace
{
	const char* const kFramePath = "data/UI/Menu/frame9.png";
	const char* const kSelectBasePath = "data/UI/Menu/select_base.png";
	const char* const kHalftonePath = "data/UI/Menu/halftone.png";
	const char* const kGlowPath = "data/UI/Menu/glow.png";
	const char* const kBracketPath = "data/UI/Menu/bracket.png";
	const char* const kLineFadePath = "data/UI/Menu/line_fade.png";
	const char* const kGridPath = "data/UI/Menu/grid.png";
	const char* const kRainPath = "data/UI/Menu/rain.png";
	const char* const kVignettePath = "data/UI/Menu/vignette.png";
	const char* const kButtonAPath = "data/UI/button/A.png";

	constexpr int kTitleFontSize = 22;
	constexpr int kItemFontSize = 22;
	constexpr int kSmallFontSize = 17;
	constexpr int kFontThick = 6;

	//色
	constexpr int kRedR = 230, kRedG = 45, kRedB = 70;
	constexpr int kBgR = 14, kBgG = 8, kBgB = 16;

	//枠(9スライス)
	constexpr int kFrameSize = 48;
	constexpr int kFrameCorner = 16;

	//背景
	constexpr float kGridTile = 64.0f;
	constexpr int kGridAlpha = 14;
	constexpr float kRainTile = 256.0f;
	constexpr float kRainSpeed = 0.004f;//1フレームに流れる量(タイル単位)
	constexpr int kRainAlpha = 170;

	//タイトル
	constexpr float kTitleLineY = 40.0f;
	constexpr float kTitleLineX1 = 70.0f;
	constexpr float kTitleLineX2 = 1210.0f;
	constexpr float kTitleTextY = 50.0f;
	constexpr int kTitleSpace = 6;

	//左のパネル
	constexpr float kListX1 = 57.0f, kListY1 = 117.0f, kListX2 = 417.0f, kListY2 = 623.0f;
	constexpr float kListHeaderY1 = 125.0f, kListHeaderY2 = 151.0f;
	constexpr float kListIconX1 = 65.0f, kListIconX2 = 91.0f;
	constexpr float kListHeaderX1 = 96.0f, kListHeaderX2 = 409.0f;
	constexpr float kItemX = 92.0f;
	constexpr float kItemBracketX = 64.0f;
	constexpr float kItemStartY = 172.0f;
	constexpr float kItemPitch = 44.0f;
	constexpr float kItemHeight = 36.0f;
	constexpr float kSelectX1 = 66.0f, kSelectX2 = 412.0f;

	//右のパネル
	constexpr float kDetailX1 = 460.0f, kDetailY1 = 117.0f, kDetailX2 = 1037.0f, kDetailY2 = 493.0f;
	constexpr float kDetailPad = 16.0f;
	constexpr float kDetailNameY = 168.0f;
	constexpr float kDetailTextY = 222.0f;
	constexpr float kDetailLineHeight = 30.0f;

	//下のボタン説明
	constexpr float kGuideY = 676.0f;
	constexpr float kGuideLineX1 = 380.0f, kGuideLineX2 = 900.0f;
	constexpr float kGuideIconSize = 24.0f;

	//選択帯のアニメ
	constexpr int kSelectAnimFrame = 10;
	constexpr int kFlickerFrame = 4;
	constexpr float kGlowPulseSpeed = 0.08f;
}

MenuUI::~MenuUI()
{
	DeleteGraph(m_frameHandle);
	DeleteGraph(m_selectBaseHandle);
	DeleteGraph(m_halftoneHandle);
	DeleteGraph(m_glowHandle);
	DeleteGraph(m_bracketHandle);
	DeleteGraph(m_lineFadeHandle);
	DeleteGraph(m_gridHandle);
	DeleteGraph(m_rainHandle);
	DeleteGraph(m_vignetteHandle);
	DeleteGraph(m_buttonAHandle);
	DeleteFontToHandle(m_titleFont);
	DeleteFontToHandle(m_itemFont);
	DeleteFontToHandle(m_smallFont);
}

void MenuUI::Init()
{
	m_frameHandle = LoadGraph(kFramePath);
	m_selectBaseHandle = LoadGraph(kSelectBasePath);
	m_halftoneHandle = LoadGraph(kHalftonePath);
	m_glowHandle = LoadGraph(kGlowPath);
	m_bracketHandle = LoadGraph(kBracketPath);
	m_lineFadeHandle = LoadGraph(kLineFadePath);
	m_gridHandle = LoadGraph(kGridPath);
	m_rainHandle = LoadGraph(kRainPath);
	m_vignetteHandle = LoadGraph(kVignettePath);
	m_buttonAHandle = LoadGraph(kButtonAPath);

	const float scale = Game::GetScale();
	m_titleFont = UIFont::Create(static_cast<int>(kTitleFontSize * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_4X4);
	m_itemFont = UIFont::Create(static_cast<int>(kItemFontSize * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_4X4);
	m_smallFont = UIFont::Create(static_cast<int>(kSmallFontSize * scale), kFontThick, DX_FONTTYPE_ANTIALIASING_4X4);
	SetFontSpaceToHandle(static_cast<int>(kTitleSpace * scale), m_titleFont);
	SetFontSpaceToHandle(static_cast<int>(3 * scale), m_itemFont);
	SetFontSpaceToHandle(static_cast<int>(1 * scale), m_smallFont);
}

void MenuUI::Update()
{
	m_frame++;
	m_selectFrame++;
}

void MenuUI::SetCursor(int cursor)
{
	if (cursor == m_cursor) return;
	m_cursor = cursor;
	m_selectFrame = 0;
}

void MenuUI::SetDetail(const std::string& header, const std::string& name, const std::string& text)
{
	m_detailHeader = header;
	m_detailName = name;
	m_detailText = text;
}

void MenuUI::Draw()
{
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawBackground();
	DrawTitle();
	DrawListPanel();
	DrawDetailPanel();
	DrawGuide();
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
}

void MenuUI::DrawBackground()
{
	const float w = static_cast<float>(Game::kScreenWidth);
	const float h = static_cast<float>(Game::kScreenHeight);
	DrawBox(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), GetColor(kBgR, kBgG, kBgB), TRUE);

	//斜めの格子(加算でうっすら)
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
	DrawTiled(m_gridHandle, 0, 0, w, h, kGridTile, 0.0f, 0.0f, kRedR, kRedG, kRedB, kGridAlpha);
	//流れるドット
	DrawTiled(m_rainHandle, 0, 0, w, h, kRainTile, 0.0f, -m_frame * kRainSpeed, kRedR, kRedG, kRedB, kRainAlpha);

	//周辺減光(乗算)
	SetDrawBlendMode(DX_BLENDMODE_MUL, 255);
	DrawExtendGraph(0, 0, Game::GetScreenWidth(), Game::GetScreenHeight(), m_vignetteHandle, FALSE);
}

void MenuUI::DrawTitle()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	SetDrawBright(kRedR, kRedG, kRedB);
	DrawExtendGraph(ToScreenX(kTitleLineX1), ToScreenY(kTitleLineY - 2), ToScreenX(kTitleLineX2), ToScreenY(kTitleLineY + 2), m_lineFadeHandle, TRUE);
	SetDrawBright(255, 255, 255);

	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	//両端の丸と中央の菱形
	DrawCircleAA(static_cast<float>(ToScreenX(kTitleLineX1)), static_cast<float>(ToScreenY(kTitleLineY)), 4.0f * Game::GetScale(), 16, red, FALSE);
	DrawCircleAA(static_cast<float>(ToScreenX(kTitleLineX2)), static_cast<float>(ToScreenY(kTitleLineY)), 4.0f * Game::GetScale(), 16, red, FALSE);
	const float cx = static_cast<float>(Game::GetScreenWidth()) * 0.5f;
	const float cy = static_cast<float>(ToScreenY(kTitleLineY));
	const float d = 5.0f * Game::GetScale();
	DrawQuadrangleAA(cx, cy - d * 1.6f, cx + d, cy, cx, cy + d * 1.6f, cx - d, cy, red, TRUE);

	const int width = GetDrawStringWidthToHandle(m_title.c_str(), static_cast<int>(m_title.size()), m_titleFont);
	DrawStringToHandle(static_cast<int>(cx) - width / 2, ToScreenY(kTitleTextY), m_title.c_str(), red, m_titleFont);
}

void MenuUI::DrawListPanel()
{
	DrawFrame(kListX1, kListY1, kListX2, kListY2, kRedR, kRedG, kRedB);

	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	//見出しの左のアイコン(仮で小さい四角を並べる)
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawBox(ToScreenX(kListIconX1), ToScreenY(kListHeaderY1), ToScreenX(kListIconX2), ToScreenY(kListHeaderY2), red, FALSE);
	const float cell = (kListIconX2 - kListIconX1) / 4.0f;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if ((i * 3 + j * 5) % 3 == 0) continue;
			const float x = kListIconX1 + cell * i + 2.0f;
			const float y = kListHeaderY1 + cell * j + 2.0f;
			DrawBox(ToScreenX(x), ToScreenY(y), ToScreenX(x + cell - 3.0f), ToScreenY(y + cell - 3.0f), red, TRUE);
		}
	}
	//見出しの帯
	DrawBox(ToScreenX(kListHeaderX1), ToScreenY(kListHeaderY1), ToScreenX(kListHeaderX2), ToScreenY(kListHeaderY2), GetColor(205, 40, 62), TRUE);
	DrawStringToHandle(ToScreenX(kListHeaderX1 + 14.0f), ToScreenY(kListHeaderY1 + 4.0f), m_listHeader.c_str(), GetColor(255, 215, 220), m_smallFont);

	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		const float y = kItemStartY + kItemPitch * i;
		const bool isSelected = (i == m_cursor);
		if (isSelected)
		{
			DrawSelectBar(kSelectX1, y, kSelectX2 - kSelectX1, kItemHeight);
		}
		else
		{
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
			SetDrawBright(kRedR, kRedG, kRedB);
			DrawExtendGraph(ToScreenX(kItemBracketX), ToScreenY(y), ToScreenX(kItemBracketX + 12.0f), ToScreenY(y + kItemHeight), m_bracketHandle, TRUE);
			SetDrawBright(255, 255, 255);
		}
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
		const unsigned int color = isSelected ? GetColor(255, 236, 238) : red;
		DrawStringToHandle(ToScreenX(kItemX), ToScreenY(y + 6.0f), m_items[i].c_str(), color, m_itemFont);
	}
}

void MenuUI::DrawSelectBar(float x, float y, float w, float h)
{
	const float t = (std::min)(1.0f, static_cast<float>(m_selectFrame) / kSelectAnimFrame);
	const float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	const float barW = w * ease;
	//出始めはちらつかせる
	const float flicker = (m_selectFrame < kFlickerFrame && (m_selectFrame % 2) == 1) ? 0.4f : 1.0f;

	//光(加算)
	const float pulse = 0.5f + 0.5f * std::sin(m_frame * kGlowPulseSpeed);
	SetDrawBlendMode(DX_BLENDMODE_ADD, static_cast<int>((35 + 30 * pulse) * flicker));
	SetDrawBright(kRedR, kRedG / 2, kRedB / 2);
	DrawExtendGraph(ToScreenX(x - 10.0f), ToScreenY(y - 8.0f), ToScreenX(x + barW + 10.0f), ToScreenY(y + h + 8.0f), m_glowHandle, TRUE);

	//ベース(左が濃い向きに反転して描く)
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(220 * flicker));
	SetDrawBright(kRedR, kRedG, kRedB);
	const float sx1 = static_cast<float>(ToScreenX(x));
	const float sx2 = static_cast<float>(ToScreenX(x + barW));
	const float sy1 = static_cast<float>(ToScreenY(y));
	const float sy2 = static_cast<float>(ToScreenY(y + h));
	DrawModiGraphF(sx2, sy1, sx1, sy1, sx1, sy2, sx2, sy2, m_selectBaseHandle, TRUE);
	SetDrawBright(255, 255, 255);

	//網点を暗く重ねる
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	DrawTiled(m_halftoneHandle, x, y, x + barW, y + h, 6.0f, 0.0f, 0.0f, 40, 0, 10, 90);

	//左端の明るい縦線
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, static_cast<int>(255 * flicker));
	DrawBox(ToScreenX(x), ToScreenY(y), ToScreenX(x + 3.0f), ToScreenY(y + h), GetColor(255, 90, 110), TRUE);
}

void MenuUI::DrawDetailPanel()
{
	DrawFrame(kDetailX1, kDetailY1, kDetailX2, kDetailY2, kRedR, kRedG, kRedB);

	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	const float hx = kDetailX1 + kDetailPad;
	const float hy = kDetailY1 + 10.0f;
	DrawStringToHandle(ToScreenX(hx), ToScreenY(hy), m_detailHeader.c_str(), red, m_smallFont);
	//見出しから右へ伸びる線
	const int headerW = GetDrawStringWidthToHandle(m_detailHeader.c_str(), static_cast<int>(m_detailHeader.size()), m_smallFont);
	const int lineY = ToScreenY(hy + 10.0f);
	const int lineX2 = ToScreenX(kDetailX2 - kDetailPad);
	DrawLine(ToScreenX(hx) + headerW + ToScreenX(8.0f), lineY, lineX2, lineY, red);
	DrawCircleAA(static_cast<float>(lineX2), static_cast<float>(lineY), 3.5f * Game::GetScale(), 12, red, FALSE);

	DrawStringToHandle(ToScreenX(hx + 8.0f), ToScreenY(kDetailNameY), m_detailName.c_str(), GetColor(255, 236, 238), m_itemFont);
	SetDrawBright(kRedR, kRedG, kRedB);
	DrawExtendGraph(ToScreenX(hx), ToScreenY(kDetailNameY + 36.0f), ToScreenX(kDetailX2 - kDetailPad), ToScreenY(kDetailNameY + 39.0f), m_lineFadeHandle, TRUE);
	SetDrawBright(255, 255, 255);

	DrawWrapText(hx + 8.0f, kDetailTextY, kDetailX2 - kDetailX1 - kDetailPad * 2 - 16.0f, kDetailLineHeight, m_detailText, GetColor(220, 150, 160), m_smallFont);
}

void MenuUI::DrawGuide()
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	SetDrawBright(kRedR, kRedG, kRedB);
	DrawExtendGraph(ToScreenX(kGuideLineX1), ToScreenY(kGuideY - 1), ToScreenX(kGuideLineX2), ToScreenY(kGuideY + 2), m_lineFadeHandle, TRUE);
	SetDrawBright(255, 255, 255);

	const unsigned int red = GetColor(kRedR, kRedG, kRedB);
	const float cx = Game::kScreenWidth * 0.5f;
	const float iconX = cx - 110.0f;
	const float iconY = kGuideY - kGuideIconSize * 0.5f;
	//背景を消すので先に暗い板
	DrawBox(ToScreenX(iconX - 12.0f), ToScreenY(iconY - 2.0f), ToScreenX(cx + 120.0f), ToScreenY(iconY + kGuideIconSize + 2.0f), GetColor(kBgR, kBgG, kBgB), TRUE);
	DrawExtendGraph(ToScreenX(iconX), ToScreenY(iconY), ToScreenX(iconX + kGuideIconSize), ToScreenY(iconY + kGuideIconSize), m_buttonAHandle, TRUE);
	DrawStringToHandle(ToScreenX(iconX + kGuideIconSize + 6.0f), ToScreenY(iconY + 2.0f), "決定", red, m_smallFont);
	DrawStringToHandle(ToScreenX(cx + 10.0f), ToScreenY(iconY + 2.0f), "↑↓ 選択", red, m_smallFont);
}

void MenuUI::DrawFrame(float x1, float y1, float x2, float y2, int r, int g, int b)
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	SetDrawBright(r, g, b);
	const float c = static_cast<float>(kFrameCorner);
	const int mid = kFrameSize - kFrameCorner * 2;
	const float xs[4] = { x1, x1 + c, x2 - c, x2 };
	const float ys[4] = { y1, y1 + c, y2 - c, y2 };
	const int srcPos[3] = { 0, kFrameCorner, kFrameSize - kFrameCorner };
	const int srcLen[3] = { kFrameCorner, mid, kFrameCorner };
	for (int iy = 0; iy < 3; ++iy)
	{
		for (int ix = 0; ix < 3; ++ix)
		{
			if (ix == 1 && iy == 1) continue;//中は透明
			DrawRectExtendGraph(ToScreenX(xs[ix]), ToScreenY(ys[iy]), ToScreenX(xs[ix + 1]), ToScreenY(ys[iy + 1]),
				srcPos[ix], srcPos[iy], srcLen[ix], srcLen[iy], m_frameHandle, TRUE);
		}
	}
	SetDrawBright(255, 255, 255);
}

void MenuUI::DrawTiled(int handle, float x1, float y1, float x2, float y2, float tileSize, float u, float v, int r, int g, int b, int a)
{
	if (x2 <= x1 || y2 <= y1) return;
	const float u2 = u + (x2 - x1) / tileSize;
	const float v2 = v + (y2 - y1) / tileSize;
	const float sx1 = static_cast<float>(ToScreenX(x1));
	const float sx2 = static_cast<float>(ToScreenX(x2));
	const float sy1 = static_cast<float>(ToScreenY(y1));
	const float sy2 = static_cast<float>(ToScreenY(y2));
	const COLOR_U8 dif = GetColorU8(r, g, b, a);

	VERTEX2D vtx[6];
	const float px[6] = { sx1, sx2, sx1, sx2, sx2, sx1 };
	const float py[6] = { sy1, sy1, sy2, sy1, sy2, sy2 };
	const float pu[6] = { u, u2, u, u2, u2, u };
	const float pv[6] = { v, v, v2, v, v2, v2 };
	for (int i = 0; i < 6; ++i)
	{
		vtx[i].pos = VGet(px[i], py[i], 0.0f);
		vtx[i].rhw = 1.0f;
		vtx[i].dif = dif;
		vtx[i].u = pu[i];
		vtx[i].v = pv[i];
	}
	SetTextureAddressMode(DX_TEXADDRESS_WRAP);
	DrawPolygon2D(vtx, 2, handle, TRUE);
	SetTextureAddressMode(DX_TEXADDRESS_CLAMP);
}

void MenuUI::DrawWrapText(float x, float y, float width, float lineHeight, const std::string& text, unsigned int color, int font)
{
	const int maxW = ToScreenX(x + width) - ToScreenX(x);
	size_t lineStart = 0;
	size_t pos = 0;
	int line = 0;
	while (pos < text.size())
	{
		//Shift-JISの2バイト文字
		const unsigned char c = static_cast<unsigned char>(text[pos]);
		const size_t len = ((c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC)) ? 2 : 1;
		const size_t next = (std::min)(pos + len, text.size());
		const int w = GetDrawStringWidthToHandle(text.c_str() + lineStart, static_cast<int>(next - lineStart), font);
		if (w > maxW && pos > lineStart)
		{
			const std::string str = text.substr(lineStart, pos - lineStart);
			DrawStringToHandle(ToScreenX(x), ToScreenY(y + lineHeight * line), str.c_str(), color, font);
			line++;
			lineStart = pos;
		}
		pos = next;
	}
	if (lineStart < text.size())
	{
		const std::string str = text.substr(lineStart);
		DrawStringToHandle(ToScreenX(x), ToScreenY(y + lineHeight * line), str.c_str(), color, font);
	}
}

int MenuUI::ToScreenX(float x) const
{
	return static_cast<int>(Game::ScaleX(x));
}

int MenuUI::ToScreenY(float y) const
{
	return static_cast<int>(Game::ScaleY(y));
}
