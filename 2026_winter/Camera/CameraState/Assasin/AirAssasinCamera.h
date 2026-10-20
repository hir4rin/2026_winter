#pragma once
#include "../CameraStateBase.h"
class AirAssasinCameraState :
    public CameraStateBase
{
public:
	AirAssasinCameraState(std::weak_ptr<CameraManager> owner);
	virtual ~AirAssasinCameraState();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::AirAssasinCamera; }
	virtual BlendSetting GetBlendSetting()const override;
	BlendSetting GetOverrideBlendSetting()const override;
private:
	float m_goalAngleH = 0.0f;//PlayerFollowCameraに渡す水平角度
	float m_goalAngleV = 0.0f;//PlayerFollowCameraに渡す垂直角度

};
