#pragma once
#include "Camera/CameraState/CameraStateBase.h"
#include "PlayerStartMovieFrame.h"

//登場演出のC8//背中側へ回り込んでPlayerFollowCameraの初期位置へ
class PlayerStartMovieTurnCamera :
    public CameraStateBase
{
public:
	PlayerStartMovieTurnCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame);
	virtual ~PlayerStartMovieTurnCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	Type GetCameraType()const override { return Type::PlayerStartMovieTurnCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	void CalcGoal();

private:
	PlayerStartMovieFrame m_frame;
	float m_frameCount = 0.0f;
};
