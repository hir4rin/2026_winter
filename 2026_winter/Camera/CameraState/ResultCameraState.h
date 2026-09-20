#pragma once
#include "CameraStateBase.h"

//リザルト画面(GameClearScene)で、プレイヤーを定点で映すカメラ
class ResultCameraState : public CameraStateBase
{
public:
	ResultCameraState(std::weak_ptr<CameraManager> owner);
	virtual ~ResultCameraState();

	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void CameraSetting() override;

	Type GetCameraType()const override { return Type::ResultCamera; }
	virtual BlendSetting GetBlendSetting()const override
	{
		return BlendSetting{
			.mode = BlendSetting::Mode::None,
			.duration = 0.0f,
			.easingPower = 1.0f,
			.pivot = Vector3()
		};
	}
};
