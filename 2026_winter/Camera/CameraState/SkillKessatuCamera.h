#pragma once
#include "CameraStateBase.h"
class SkillKessatuCamera :
    public CameraStateBase
{
public:
	SkillKessatuCamera(std::weak_ptr<CameraManager> owner);
	virtual ~SkillKessatuCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::SkillKessatuCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	//敵の位置を更新する//敵を見失ったら最後の位置のままにする
	void UpdateEnemyPos();

	Vector3 m_enemyPos;
	bool m_isTargetLost = false;
};

