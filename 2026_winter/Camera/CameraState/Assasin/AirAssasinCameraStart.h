#pragma once
#include "../CameraStateBase.h"

/// <summary>
/// 空中暗殺のカメラ(敵→プレイヤーの延長線から5度ずらした位置)
/// 視野角をだんだん狭くする。PlayerStateAirAssasinでAirAssasinCameraStateへSlerp(EaseOutBack)で遷移する
/// </summary>
class AirAssasinCameraStart :
	public CameraStateBase
{
public:
	AirAssasinCameraStart(std::weak_ptr<CameraManager> owner);
	virtual ~AirAssasinCameraStart();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::AirAssasinCameraStart; }
	virtual BlendSetting GetBlendSetting()const override;
	//次のState(AirAssasinCameraState)へ渡す上書きブレンド(プレイヤーを中心にSlerp)
	BlendSetting GetOverrideBlendSetting()const override;
private:
	float m_timer = 0.0f;
	float m_sideSign = 1.0f;//ずらす向き(1:右 -1:左)
	Vector3 m_forward = Vector3(0.0f, 0.0f, 1.0f);//プレイヤー→敵(水平)
};
