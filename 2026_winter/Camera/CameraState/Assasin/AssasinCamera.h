#pragma once
#include "../CameraStateBase.h"
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
	BlendSetting GetOverrideBlendSetting()const override;
private:
	float m_timer = 0.0f;
	float m_distanceTimer = 0.0f;//距離を変え始めてからの経過フレーム数
	float m_distance = 0.0f;//プレイヤーから敵方向へのカメラの水平距離

};
