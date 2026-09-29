#pragma once
#include "CameraStateBase.h"
class AssasinCameraState :
    public CameraStateBase
{
public:
	AssasinCameraState(std::weak_ptr<CameraManager> owner);
	virtual ~AssasinCameraState();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::AssasinCamera; }
	virtual BlendSetting GetBlendSetting()const override;

};
