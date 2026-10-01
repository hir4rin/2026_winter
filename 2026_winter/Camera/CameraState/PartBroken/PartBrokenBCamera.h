#pragma once
#include "../CameraStateBase.h"
class PartBrokenBCamera :
    public CameraStateBase
{
public:
	PartBrokenBCamera(std::weak_ptr<CameraManager> owner);
	virtual ~PartBrokenBCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::PartBrokenCameraB; }
	virtual BlendSetting GetBlendSetting()const override;
	//次のState(AssasinCameraState)へ渡す上書きブレンド(プレイヤーを中心にSlerp)
	BlendSetting GetOverrideBlendSetting()const override;
private:
	float m_timer = 0.0f;
	float m_distanceTimer = 0.0f;//距離を変え始めてからの経過フレーム数
	float m_distance = 0.0f;//プレイヤーから敵方向へのカメラの水平距離
	Vector3 m_startPtoEDir = Vector3(0.0f, 0.0f, 1.0f);//開始時のプレイヤー→敵の水平方向(正規化済み)
};

