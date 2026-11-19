#pragma once
#include "CameraStateBase.h"
#include <cmath>

//エンディングのカメラ//プレイヤーの後ろの低い所から、進む向きの空を見上げる
class EndingCamera :
    public CameraStateBase
{
public:
	//プレイヤーの足元から見た置き方
	struct Shot
	{
		float back = 0.0f;//後ろへの距離
		float height = 0.0f;//カメラの高さ
		float targetAhead = 0.0f;//注視点の前への距離
		float targetHeight = 0.0f;//注視点の高さ
		float fov = 60.0f;//度
	};

	EndingCamera(std::weak_ptr<CameraManager> owner);
	virtual ~EndingCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	//移動の入力もこの向きが基準になる
	void SetForward(const Vector3& forward) { m_forward = forward; m_angleH = atan2f(forward.x, forward.z); }
	//今の置き方から frame かけて shot へ動かす
	void MoveTo(const Shot& shot, float frame);

	Type GetCameraType()const override { return Type::EndingCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	Shot CurrentShot()const;

private:
	Vector3 m_forward = Vector3(0.0f, 0.0f, 1.0f);
	Shot m_fromShot;
	Shot m_toShot;
	float m_shotFrame = 0.0f;
	float m_shotLength = 1.0f;
};
