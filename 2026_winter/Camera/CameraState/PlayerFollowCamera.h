#pragma once
#include "CameraStateBase.h"

class PlayerFollowCamera :public CameraStateBase
{
public:
	PlayerFollowCamera(std::weak_ptr<CameraManager> owner);
	virtual ~PlayerFollowCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::PlayerCamera; }
	virtual BlendSetting GetBlendSetting()const override
	{
		return BlendSetting{
			.mode = BlendSetting::Mode::Lerp,
			.duration = kBlendDuration,
			.easingPower = 1.0f,
			.pivot = Vector3()
		};
	}

private:
	void InputRightStick();//右スティックの入力を処理する
	void FixCameraPosLockOn();//ロックオン時のカメラの位置を調整する
private:
	static constexpr float kBlendDuration = 5.0f;//ブレンドにかけるフレーム数

	XINPUT_STATE  xi;
	Vector3 m_rayVec = Vector3(0, 0, 0);//カメラの前方向のベクトル//カメラの注視点を決めるために使う

	Vector3 m_testPos = Vector3(0, 0, 0);
	Vector3 m_testPos2 = Vector3(0, 0, 0);
};

