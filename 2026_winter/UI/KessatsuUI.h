#pragma once
#include "UIBase.h"
#include <array>

//必殺技(血殺)中に「血」「殺」の文字と血の液体、白い爆発を表示するUI
//2026_summer_refactoring の GameScene::DrawUltKessatsuUI を移植したもの
class KessatsuUI : public UIBase
{
public:
	KessatsuUI();
	~KessatsuUI() override;

	void Init() override;
	void Update() override;
	void Draw() override;
	//敵とプレイヤーより後ろに出す
	UILayer GetLayer() const override { return UILayer::Back; }

private:
	//必殺技演出時の配置(血:右上寄り、殺:右下寄り、液体:左下寄り)
	void DrawUlt();

	//設計解像度(1280x720)の座標・拡大率で画像を描画する(実解像度に合わせて拡大する)
	void DrawDesignGraph(float x, float y, float scale, float angle, int handle);

	//白い爆発を始める
	void StartBurst();
	//白い爆発を描く
	void DrawBurst();
	//画面のふちを暗くする墨を描く
	void DrawVignette();
	//文字と爆発の透明度(演出が終わったらビネットと一緒に消える)
	int GetFadeAlpha()const;
	//斬撃の線の角度・長さをランダムに決める
	void StartSlash();
	//白い爆発の中心から抜ける斬撃の線を描く
	void DrawSlash();

	int m_bloodHandle = -1;//「血」
	int m_splashHandle = -1;//血の液体
	int m_satsuHandle = -1;//「殺」

	static constexpr int kBurstImageNum = 3;
	std::array<int, kBurstImageNum> m_burstHandles = { -1, -1, -1 };//白い爆発
	int m_burstIndex = 0;//使う画像
	int m_burstFrame = -1;//経過フレーム(-1:出ていない)
	float m_burstAngle = 0.0f;
	float m_burstX = 0.0f;//画面の位置
	float m_burstY = 0.0f;
	bool m_wasUltimating = false;

	int m_vignetteHandle = -1;//墨のビネット
	float m_vignetteRate = 0.0f;//かかり具合(0～1)

	//斬撃の線1本分
	struct SlashLine
	{
		float angle = 0.0f;//爆発の向きからずらす角度
		float sideOffset = 0.0f;//線の横へずらす距離
		float length = 1.0f;//長さの倍率
		float thick = 1.0f;//太さの倍率
		bool isFlip = false;//上下反転
	};
	static constexpr int kSlashLineNum = 3;
	std::array<SlashLine, kSlashLineNum> m_slashLines = {};
	int m_slashHandle = -1;
};
