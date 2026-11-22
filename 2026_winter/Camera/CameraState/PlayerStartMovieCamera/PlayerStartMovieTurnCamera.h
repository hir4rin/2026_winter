#pragma once
#include "Camera/CameraState/CameraStateBase.h"
#include "PlayerStartMovieFrame.h"

/// <summary>
/// 登場演出のC8(プレイヤーを中心にSlerpで背中側へ回り込む)
/// 着いたらPlayerFollowCameraへ(PlayerFollowCameraの初期位置に合わせてある)
/// </summary>
class PlayerStartMovieTurnCamera :
    public CameraStateBase
{
public:
	PlayerStartMovieTurnCamera(std::weak_ptr<CameraManager> owner, const PlayerStartMovieFrame& frame);
	virtual ~PlayerStartMovieTurnCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::PlayerStartMovieTurnCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	PlayerStartMovieFrame m_frame;
	float m_frameCount = 0.0f;
};
