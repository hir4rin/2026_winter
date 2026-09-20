#pragma once
#include "CameraStateBase.h"

//ボス撃破後の演出用カメラ(2つ目:血殺を打つ瞬間を映す)
class FinishingSecondCamera : public CameraStateBase
{
public:
	FinishingSecondCamera(std::weak_ptr<CameraManager> owner);
	virtual ~FinishingSecondCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::FinishingSecondCamera; }
	virtual BlendSetting GetBlendSetting()const override
	{
		return BlendSetting{
			.mode = BlendSetting::Mode::Lerp,
			.duration = kBlendDuration,
			.easingPower = kBlendEasingPower,
			.pivot = Vector3()
		};
	}
	BlendSetting GetOverrideBlendSetting()const override
	{
		return BlendSetting{
		.mode = BlendSetting::Mode::Lerp,
		.duration = kOverrideBlendDuration,
		.easingPower = kOverrideBlendEasingPower,
		.pivot = Vector3()

		};
	}
private:
	static constexpr float kBlendDuration = 40.0f;//ブレンドにかけるフレーム数
	static constexpr float kBlendEasingPower = 0.4f;//ブレンドのイージング指数

	static constexpr float kOverrideBlendDuration = 30.0f;//上書き用ブレンドにかけるフレーム数
	static constexpr float kOverrideBlendEasingPower = 1.0f;//上書き用ブレンドのイージング指数

	float m_timer = 0.0f;//演出開始からの経過フレーム//一定フレームで元のStateに戻すのに使う
};
