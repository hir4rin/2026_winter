#pragma once
#include "CameraStateBase.h"

class EnemyBase;

class LockOnCameraState : public CameraStateBase
{
public:
	LockOnCameraState(std::weak_ptr<CameraManager> owner);
	virtual ~LockOnCameraState();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void Draw() override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::LockOnCamera; }
	virtual BlendSetting GetBlendSetting()const override
	{
		return BlendSetting{
			.mode = BlendSetting::Mode::Lerp,
			.duration = kBlendDuration,
			.easingPower = kBlendEasingPower,
			.pivot = Vector3()
		};
	}

private:
	static constexpr float kBlendDuration = 25.0f;//ブレンドにかけるフレーム数
	static constexpr float kBlendEasingPower = 0.3f;//ブレンドのイージング指数

	std::weak_ptr<EnemyBase> m_lastTargetEnemy;//前フレームでロックオンしていた敵(切り替え検知用)
	Vector3 m_targetLerpStart;//注視点lerpの開始位置
	float m_targetLerpElapsed = 0.0f;//注視点lerpの経過フレーム

	int m_raticleHandle = -1;//ロックオンレティクルの画像ハンドル
};

