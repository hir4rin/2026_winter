#pragma once
#include "../CameraStateBase.h"
class PartBrokenACameraEnd :
    public CameraStateBase
{
public:
	PartBrokenACameraEnd(std::weak_ptr<CameraManager> owner);
	virtual ~PartBrokenACameraEnd();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::PartBrokenCameraAEnd; }
	virtual BlendSetting GetBlendSetting()const override;
	virtual BlendSetting GetOverrideBlendSetting()const override;
private:
	float m_goalAngleH = 0.0f;//PlayerFollowCameraに渡す水平角度
	float m_goalAngleV = 0.0f;//PlayerFollowCameraに渡す垂直角度
};

