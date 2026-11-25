#pragma once
#include "UI/UIBase.h"
#include <memory>

class CameraManager;

//RTで次の目的地を見ている間の画面効果(色を抜く・墨のふち・入りの放射ブラー)
//3Dの後・ほかのUIより先に描く(UIManagerの登録の順番)
class LookNextFxUI : public UIBase
{
public:
	explicit LookNextFxUI(std::weak_ptr<CameraManager> cameraManager);
	~LookNextFxUI() override;

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	bool IsLooking() const;
	bool IsBlocked() const;//演出中はかけない
	//目的地の画面の位置(UV)。カメラの後ろなら false
	bool CalcTargetUv(float& u, float& v) const;

	struct FxConstant
	{
		float param[4];//効き具合・入りのブラー・横/縦
		float target[4];//目的地(UV)
	};

	std::weak_ptr<CameraManager> m_cameraManager;
	int m_shader = -1;
	int m_constantBuffer = -1;
	int m_screen = -1;//今の画面の写し
	int m_inkHandle = -1;
	float m_rate = 0.0f;//0〜1(実フレームで進む)
	bool m_wasLooking = false;
	int m_pulseFrame = -1;//入ってからのフレーム(-1なら無し)
	float m_targetU = 0.5f;
	float m_targetV = 0.5f;
};
