#pragma once
#include "CameraStateBase.h"
class UltCameraState :
    public CameraStateBase
{
public:
	UltCameraState(std::weak_ptr<CameraManager> owner);
	virtual ~UltCameraState();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::UltCamera; }
	virtual BlendSetting GetBlendSetting()const override;

};

