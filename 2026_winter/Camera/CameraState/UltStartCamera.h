#pragma once
#include "CameraStateBase.h"

/// <summary>
/// ウルト演出の最初のカメラ
/// </summary>
class UltStartCamera :
	public CameraStateBase
{
public:
	UltStartCamera(std::weak_ptr<CameraManager> owner);
	virtual ~UltStartCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::UltStartCamera; }
	virtual BlendSetting GetBlendSetting()const override;
private:
	float m_timer = 0.0f;
};
