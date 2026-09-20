#pragma once
#include "CameraStateBase.h"

//ボス登場時の演出用カメラ
class BossAppearCamera : public CameraStateBase
{
public:
	BossAppearCamera(std::weak_ptr<CameraManager> owner);
	virtual ~BossAppearCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::BossAppearCamera; }
	virtual BlendSetting GetBlendSetting()const override
	{
		//TODO: ここは自分で調整する
		return BlendSetting{
			.mode = BlendSetting::Mode::None,
			.duration = 0.0f,
			.easingPower = 1.0f,
			.pivot = Vector3()
		};
	}

private:
	float m_timer = 0.0f;//演出開始からの経過フレーム//一定フレームで元のStateに戻すのに使う
};
