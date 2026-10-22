#pragma once
#include "CameraStateBase.h"

//バトル中、PlayerFollowCameraで攻撃がヒットしたときに、
//敵→プレイヤーの軸から近いほうに45度傾けた位置へ回り込むカメラ
//右スティックの入力・歩き出した・敵から離れた・空中の敵が近すぎるときはPlayerFollowCameraに戻る
class BattleSmoothCamera :
    public CameraStateBase
{
public:
	BattleSmoothCamera(std::weak_ptr<CameraManager> owner);
	virtual ~BattleSmoothCamera();
	virtual void Enter(CameraData data)override;
	virtual void Update()override;
	virtual void Exit()override;

	void FixCameraPos() override;
	void CameraSetting() override;

	Type GetCameraType()const override { return Type::BattleSmoothCamera; }
	virtual BlendSetting GetBlendSetting()const override;

private:
	void DecideRotateSide();//どちら側に傾けるかを決める//Enterで一度だけ呼ぶ
	bool IsExitCondition(const Vector3& playerPos, const Vector3& enemyPos, bool isEnemyFloor);//PlayerFollowCameraに戻るか//timerでmove,runのフレームを数えて決める

private:
	float m_rotateAngle = 0.0f;//敵→プレイヤーベクトルを回転させる角度(符号でどちら側かを決める)
	int m_moveFrame = 0;//攻撃せずに歩いているフレーム数
};
