#pragma once
#include "../CameraStateBase.h"

/// <summary>
/// 暗殺演出の最初のカメラ(プレイヤーの右斜め後ろ)
/// 一定時間後にAssasinCameraStateへSlerp(EaseOutBack)で遷移する
/// </summary>
class AssasinCameraStart :
	public CameraStateBase
{
public:
	AssasinCameraStart(std::weak_ptr<CameraManager> owner);
	virtual ~AssasinCameraStart();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::AssasinCameraStart; }
	virtual BlendSetting GetBlendSetting()const override;
	//次のState(AssasinCameraState)へ渡す上書きブレンド(プレイヤーを中心にSlerp)
	BlendSetting GetOverrideBlendSetting()const override;
private:
	float m_timer = 0.0f;
};
