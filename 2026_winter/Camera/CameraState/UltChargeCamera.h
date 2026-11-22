#pragma once
#include "CameraStateBase.h"

//必殺技(LT2回)の構え〜溜めのカメラ
//プレイヤーの前の低い所から見上げ、刀が伸びるのに合わせて引いて画角を広げる
//入口:PlayerStateUlt::Enter//出口:StartReturnでいつものカメラの位置へ回り込んでPlayerFollowCamera
class UltChargeCamera :
	public CameraStateBase
{
public:
	UltChargeCamera(std::weak_ptr<CameraManager> owner);
	virtual ~UltChargeCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	Type GetCameraType()const override { return Type::UltChargeCamera; }
	virtual BlendSetting GetBlendSetting()const override;

	//いつものカメラの位置へ戻り始める//着いたらPlayerFollowCameraへ
	void StartReturn();

private:
	void CalcChargePos(Vector3& pos, Vector3& target, float& fov) const;//溜めの位置
	void CalcFollowPos(Vector3& pos, Vector3& target) const;//PlayerFollowCameraの初期位置

	Vector3 m_front;//プレイヤーの前(水平)//Enterで決める
	float m_frame = 0.0f;//揺れ用のフレーム数

	bool m_isReturn = false;//いつものカメラへ戻っている途中か
	float m_returnFrame = 0.0f;
	Vector3 m_returnStartPos;
	Vector3 m_returnStartTarget;
	float m_returnStartFov = kDefaultFov;
};
