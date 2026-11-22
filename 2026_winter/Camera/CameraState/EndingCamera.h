#pragma once
#include "CameraStateBase.h"
#include <cmath>

//エンディングのカメラ//プレイヤーの後ろの低い所から、進む向きの空を見上げる
class EndingCamera :
    public CameraStateBase
{
public:
	//プレイヤーの足元から見た置き方
	struct Shot
	{
		float back = 0.0f;//後ろへの距離
		float height = 0.0f;//カメラの高さ
		float targetAhead = 0.0f;//注視点の前への距離
		float targetHeight = 0.0f;//注視点の高さ
		float fov = 60.0f;//度
	};

	EndingCamera(std::weak_ptr<CameraManager> owner);
	virtual ~EndingCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	//移動の入力もこの向きが基準になる
	void SetForward(const Vector3& forward) { m_forward = forward; m_angleH = atan2f(forward.x, forward.z); }
	//今の置き方から frame かけて shot へ動かす
	void MoveTo(const Shot& shot, float frame);
	//飛び降りるとき//今の位置から frame かけて pos へ動いて止まり、プレイヤーを目で追う
	void HoldAt(const Vector3& pos, float frame);
	//飛び降りるとき//今の位置から frame かけて、プレイヤーの足元から offset の所へ寄ってついていく
	void FollowAt(const Vector3& offset, float frame);
	//縦穴を登るとき//center(縦の線)の backDir 側から見る。横の位置は動かさず、高さだけプレイヤーについていく
	//距離は startDist → endDist、プレイヤーより上の高さは startUp → endUp へ frame かけて変える(だんだん遠ざかる)
	void RiseAlong(const Vector3& center, const Vector3& backDir, float startDist, float endDist, float startUp, float endUp, float frame);

	Type GetCameraType()const override { return Type::EndingCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	Shot CurrentShot()const;

private:
	Vector3 m_forward = Vector3(0.0f, 0.0f, 1.0f);
	Shot m_fromShot;
	Shot m_toShot;
	float m_shotFrame = 0.0f;
	float m_shotLength = 1.0f;
	enum class Mode
	{
		Shot,//MoveToの置き方
		Hold,//止まって目で追う
		Follow,//ついていく
		Rise,//縦穴の後ろから、高さだけついていく
	};
	Mode m_mode = Mode::Shot;
	//Rise
	Vector3 m_riseCenter;
	Vector3 m_riseBack;
	float m_riseStartDist = 0.0f;
	float m_riseEndDist = 0.0f;
	float m_riseStartUp = 0.0f;
	float m_riseEndUp = 0.0f;
	float m_riseY = 0.0f;//なめらかにしたプレイヤーの高さ
	Vector3 m_holdPos;
	Vector3 m_followOffset;
	Vector3 m_modeFromPos;//動き始めた位置
	Vector3 m_modeFromTarget;//動き始めた注視点
	float m_modeFrame = 0.0f;
	float m_modeLength = 0.0f;
};
