#pragma once
#include "Camera/CameraState/CameraStateBase.h"
#include "PlayerStartMovieFrame.h"

//登場演出のC6//着地のポーズのまわりをゆっくり回る
class PlayerStartMovieLandCamera :
    public CameraStateBase
{
public:
	PlayerStartMovieLandCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame);
	virtual ~PlayerStartMovieLandCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	Type GetCameraType()const override { return Type::PlayerStartMovieLandCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	void CalcGoal();

private:
	PlayerStartMovieFrame m_frame;
	float m_frameCount = 0.0f;
};
