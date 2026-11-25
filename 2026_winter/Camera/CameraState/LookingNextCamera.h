#pragma once
#include "CameraStateBase.h"

//RTを長押ししている間、プレイヤーの後ろから次のチェックポイントの方向を見るカメラ
//RTを離した・次のチェックポイントが無くなったときはPlayerFollowCameraに戻る
class LookingNextCamera :
    public CameraStateBase
{
public:
	LookingNextCamera(std::weak_ptr<CameraManager> owner);
	virtual ~LookingNextCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::LookingNextCamera; }
	virtual BlendSetting GetBlendSetting()const override;
};
