#pragma once
#include "CameraStateBase.h"

//プレイヤーがボスのパーツを投げる間のカメラ
//水平はBattleSmoothCameraの角度、位置の決め方はPlayerFollowCameraと同じ
class BossPartThrowCamera :
    public CameraStateBase
{
public:
	BossPartThrowCamera(std::weak_ptr<CameraManager> owner);
	virtual ~BossPartThrowCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;

	Type GetCameraType()const override { return Type::BossPartThrowCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	void DecideAngleH(const Vector3& cameraPos);//水平角度を決める//Enterで一度だけ呼ぶ
};
