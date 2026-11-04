#pragma once
#include <string>
#include <vector>

//メニュー画面(左に項目リスト、右に詳細)
//座標は設計解像度(1280x720)で書き、描画時にScaleする
class MenuUI
{
public:
	MenuUI() = default;
	~MenuUI();

	void Init();
	void Update();
	void Draw();

	void SetTitle(const std::string& title) { m_title = title; }
	void SetListHeader(const std::string& header) { m_listHeader = header; }
	void SetItems(const std::vector<std::string>& items) { m_items = items; }
	//カーソルが変わったら選択帯のアニメをやり直す
	void SetCursor(int cursor);
	void SetDetail(const std::string& header, const std::string& name, const std::string& text);

private:
	void DrawBackground();
	void DrawTitle();
	void DrawListPanel();
	void DrawDetailPanel();
	void DrawGuide();
	void DrawSelectBar(float x, float y, float w, float h);

	//9スライスの枠
	void DrawFrame(float x1, float y1, float x2, float y2, int r, int g, int b);
	//繰り返し貼る(u,vはずらし量、タイル単位)
	void DrawTiled(int handle, float x1, float y1, float x2, float y2, float tileSize, float u, float v, int r, int g, int b, int a);
	//幅に収まるように改行して描く(Shift-JIS)
	void DrawWrapText(float x, float y, float width, float lineHeight, const std::string& text, unsigned int color, int font);

	int ToScreenX(float x) const;
	int ToScreenY(float y) const;

	std::string m_title;
	std::string m_listHeader;
	std::vector<std::string> m_items;
	std::string m_detailHeader;
	std::string m_detailName;
	std::string m_detailText;

	int m_cursor = 0;
	int m_frame = 0;
	int m_selectFrame = 0;//カーソルが変わってからのフレーム

	int m_frameHandle = -1;
	int m_selectBaseHandle = -1;
	int m_halftoneHandle = -1;
	int m_glowHandle = -1;
	int m_bracketHandle = -1;
	int m_lineFadeHandle = -1;
	int m_gridHandle = -1;
	int m_rainHandle = -1;
	int m_vignetteHandle = -1;
	int m_buttonAHandle = -1;

	int m_titleFont = -1;
	int m_itemFont = -1;
	int m_smallFont = -1;
};
