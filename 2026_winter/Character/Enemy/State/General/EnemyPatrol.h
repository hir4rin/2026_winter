#pragma once
#include "EnemyStateBase.h"
#include "../../../../Math/Vector3.h"
class EnemyPatrol :
    public EnemyStateBase
{
public:
    EnemyPatrol(std::weak_ptr<EnemyBase> owner);
    virtual ~EnemyPatrol();

    void Enter() override;

    void Update()override;
    void Exit()override;

    void DebugDraw();
protected:
	bool IsInView(const Vector3& targetPos)const;//視界内にいるかどうか//XZ平面で判定//のちのちY軸対応
    bool CanSeePlayer()const;
private:
	float m_waitTimer = 0.0f;//待機時間のタイマー
	bool m_isWaiting = false;//巡回ポイントに着いて待機中かどうか
	bool m_wasWalking = false;//前フレームで歩きアニメだったかどうか
};

