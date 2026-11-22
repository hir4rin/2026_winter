#pragma once
#include "Camera/CameraState/CameraStateBase.h"
#include "PlayerStartMovieFrame.h"

/// <summary>
/// 登場演出のC6(着地のポーズのまわりをゆっくり回る)
/// 着地のポーズが終わったらPlayerStartMovieTurnCameraへ
/// </summary>
class PlayerStartMovieLandCamera :
    public CameraStateBase
{
public:
	//delayFrame:回り始めるまで最初の位置で待つ(チュートリアルの登場で、落ちてくるのを待つ)
	PlayerStartMovieLandCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame, float delayFrame = 0.0f);
	virtual ~PlayerStartMovieLandCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::PlayerStartMovieLandCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	PlayerStartMovieFrame m_frame;
	float m_frameCount = 0.0f;
	float m_delayFrame = 0.0f;
};
