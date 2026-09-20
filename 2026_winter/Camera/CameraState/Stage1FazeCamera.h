#pragma once
#include "CameraStateBase.h"

//フェーズ終了時の演出用カメラ
class Stage1FazeCamera : public CameraStateBase
{
public:
	Stage1FazeCamera(std::weak_ptr<CameraManager> owner);
	virtual ~Stage1FazeCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::Stage1FazeCamera; }
	virtual BlendSetting GetBlendSetting()const override
	{
		return BlendSetting{
			.mode = BlendSetting::Mode::Lerp,
			.duration = kBlendDuration,
			.easingPower = 1.0f,
			.pivot = Vector3()
		};
	}
	BlendSetting GetOverrideBlendSetting()const override
	{
		return BlendSetting{
		.mode = BlendSetting::Mode::Lerp,
		.duration = kBlendDuration,
		.easingPower = 1.0f,
		.pivot = Vector3()

		};
	}
private:
	static constexpr float kBlendDuration = 30.0f;//ブレンドにかけるフレーム数

	float m_timer = 0.0f;//演出開始からの経過フレーム//一定フレームで元のStateに戻すのに使う
};
