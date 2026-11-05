#pragma once
#include "CameraStateBase.h"

//ボスのトドメのムービーのカメラ//カットごとに始まりと終わりを指定して動かす
class BossFinisherCamera :
    public CameraStateBase
{
public:
	struct Shot
	{
		Vector3 startPos;
		Vector3 endPos;
		Vector3 startTarget;
		Vector3 endTarget;
		float startFov = 60.0f;//度
		float endFov = 60.0f;
		float frame = 1.0f;
	};

	BossFinisherCamera(std::weak_ptr<CameraManager> owner);
	virtual ~BossFinisherCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void SetShot(const Shot& shot);

	Type GetCameraType()const override { return Type::BossFinisherCamera; }
	virtual BlendSetting GetBlendSetting()const override { return {}; }

private:
	void ApplyShot();

private:
	Shot m_shot;
	float m_shotFrame = 0.0f;
	bool m_hasShot = false;
};
