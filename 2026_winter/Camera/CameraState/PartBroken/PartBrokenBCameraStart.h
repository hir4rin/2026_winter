#pragma once
#include "../CameraStateBase.h"
class PartBrokenBCameraStart :
    public CameraStateBase
{
public:
	PartBrokenBCameraStart(std::weak_ptr<CameraManager> owner);
	virtual ~PartBrokenBCameraStart();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::PartBrokenCameraBStart; }
	virtual BlendSetting GetBlendSetting()const override;
	//次のState(AssasinCameraState)へ渡す上書きブレンド(プレイヤーを中心にSlerp)
	BlendSetting GetOverrideBlendSetting()const override;
private:
	float m_timer = 0.0f;
};

