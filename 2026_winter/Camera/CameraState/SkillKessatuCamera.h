#pragma once
#include "CameraStateBase.h"
class SkillKessatuCamera :
    public CameraStateBase
{
public:
	SkillKessatuCamera(std::weak_ptr<CameraManager> owner);
	virtual ~SkillKessatuCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::SkillKessatuCamera; }
	virtual BlendSetting GetBlendSetting()const override;

};

