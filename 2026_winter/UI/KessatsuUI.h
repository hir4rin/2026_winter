#pragma once
#include <memory>
#include "UIBase.h"

class CameraManager;

//必殺技(血殺)中に「血」「殺」の文字と血の液体を表示するUI
//2026_summer_refactoring の GameScene::DrawUltKessatsuUI / DrawLastHitKessatsuUI を移植したもの
class KessatsuUI : public UIBase
{
public:
	KessatsuUI(std::weak_ptr<CameraManager> cameraManager);
	~KessatsuUI() override;

	void Init() override;
	void Update(Input& input) override;
	void Draw() override;
	//文字はプレイヤーより後ろに出す
	UILayer GetLayer() const override { return UILayer::Back; }

private:
	//必殺技演出時の配置(血:右上寄り、殺:右下寄り、液体:左下寄り)
	void DrawUlt();
	//ラストヒットイベント演出時の配置(血:左上、殺:血の少し右下、液体:右下寄り)
	void DrawLastHit();
	//ラストヒットイベントのカメラ中かどうか
	bool IsLastHitEvent() const;

	//設計解像度(1280x720)の座標・拡大率で画像を描画する(実解像度に合わせて拡大する)
	void DrawDesignGraph(float x, float y, float scale, float angle, int handle);

	std::weak_ptr<CameraManager> m_cameraManager;//どちらの配置で出すかをカメラの種類で決める

	int m_bloodHandle = -1;//「血」
	int m_splashHandle = -1;//血の液体
	int m_satsuHandle = -1;//「殺」
};
