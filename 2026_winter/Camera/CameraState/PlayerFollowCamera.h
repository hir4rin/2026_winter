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
			.easingMode = BlendSetting::EasingMode::EaseIn,
			.easingPower = 0.5f,
			.pivot = Vector3()
		};
	}

private:
	void InputRightStick();//右スティックの入力を処理する

	void DragCameraByPlayerMove();//プレイヤーの移動量でカメラの回転角度を更新//その際に角度を割合で回すことで不完全だけど自然な動きになる//スプリングアーム
private:
	static constexpr float kBlendDuration = 20.0f;//ブレンドにかけるフレーム数

	XINPUT_STATE  xi;
	Vector3 m_rayVec = Vector3(0, 0, 0);//カメラの前方向のベクトル//カメラの注視点を決めるために使う

	Vector3 m_prevGoalTarget = Vector3(0, 0, 0);//前フレームの注視点

	Vector3 m_testPos = Vector3(0, 0, 0);
	Vector3 m_testPos2 = Vector3(0, 0, 0);
};

