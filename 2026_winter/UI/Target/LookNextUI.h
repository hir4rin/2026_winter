#pragma once
#include <memory>
#include "UI/UIBase.h"

class CameraManager;

//RTで次の目的地を見ている間、カメラの注視点に出す紋
class LookNextUI : public UIBase
{
public:
	LookNextUI(std::weak_ptr<CameraManager> cameraManager);

	void Init()override;
	void Update()override;
	void Draw()override;

private:
	bool IsLooking() const;//LookingNextCamera中か
	void Hide();

	float GetInRate() const;//入りの進み(0〜1)
	float GetAlpha() const;

	//x,yは中心(実解像度)
	void DrawCrest(float x, float y, float scale, float alpha);

	std::weak_ptr<CameraManager> m_cameraManager;

	int m_frame = 0;//入ってからのフレーム
	int m_totalFrame = 0;//回転用
	bool m_isFading = false;//離して消えている
	int m_fadeFrame = 0;
	Vector3 m_targetPos;//消えている間はカメラが戻っても前の注視点に出す
};
