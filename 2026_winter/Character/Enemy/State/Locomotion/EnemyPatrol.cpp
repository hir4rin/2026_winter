#include "EnemyPatrol.h"
#include "Main/System.h"
#include "Character/Player/Player.h"
#include "../../EnemyBase.h"
#include "../Math/Vector3.h"

namespace
{
	constexpr float kPatrolArriveDistance = 10.0f;//この距離(XZ)まで近づいたら到着とする//移動速度より大きくしないと行き過ぎて到着できない

	//デバッグ描画
	constexpr float kDebugPointRadius = 100.0f;//巡回ポイントの球の半径
	constexpr int kDebugPointDivNum = 16;//球の分割数
	const unsigned int kDebugPointColor = 0x00ffff;//巡回ポイントの色
	const unsigned int kDebugTargetPointColor = 0xff0000;//今向かっている巡回ポイントの色
	const unsigned int kDebugRouteLineColor = 0xffff00;//ルートの線の色
	constexpr int kDebugViewArcDivNum = 16;//視界の円弧の分割数
	constexpr float kDebugViewHeight = 10.0f;//地面に埋まらないよう少し浮かせる
	const unsigned int kDebugViewColor = 0x00ff00;//視界の色(見つけていない)
	const unsigned int kDebugViewFoundColor = 0xff0000;//視界の色(プレイヤーが視界内)
}

EnemyPatrol::EnemyPatrol(std::weak_ptr<EnemyBase> owner):EnemyStateBase(owner)
{
}

EnemyPatrol::~EnemyPatrol()
{
}

void EnemyPatrol::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下禁止を有効化
	owner->SetPreventFall(true);

	//パトロールポイントのインデックスを取得
	int currentIndex = owner->m_currentPatrolIndex;

	//設定されていないときはゼロからスタート
	if (currentIndex == -1)
	{
		owner->m_currentPatrolIndex = 0;
		currentIndex = 0;
	}
	//現在のパトロールポイントに向かって移動を始める
	auto patrolPoint = owner->m_patrolPoints[currentIndex];
	m_isWaiting = false;
	m_waitTimer = 0.0f;
	owner->ChaseTarget(patrolPoint.pos, kPatrolArriveDistance, owner->m_status.patrolSpeed);

	//開始時の速度に合わせてアニメーションを設定
	m_wasWalking = owner->GetRigidBody().m_vel.Magnitude() > 0.1f;
	const char* animKey = m_wasWalking ? "Walk" : "Idle";
	owner->m_anim.ChangeAnim(owner->GetAnimName(animKey), true, 0.7f * owner->GetAnimSpeed(animKey));
}

void EnemyPatrol::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//パトロールポイントの個数を取得
	int patrolMaxCount = static_cast<int>(owner->m_patrolPoints.size());
	
	//現在のパトロールポイントを取得
	auto patrolPoint = owner->m_patrolPoints[owner->m_currentPatrolIndex];

	if (!m_isWaiting)
	{
		//パトロールポイントに向かって移動
		owner->ChaseTarget(patrolPoint.pos, kPatrolArriveDistance, owner->m_status.patrolSpeed);

		//高さは床によってずれるので、XZだけで距離を測る
		Vector3 toPoint = patrolPoint.pos - owner->GetRigidBody().GetPos();
		toPoint.y = 0.0f;
		if (toPoint.Magnitude() <= kPatrolArriveDistance)
		{
			//着いたらこのポイントの待機時間だけ待つ
			m_isWaiting = true;
			m_waitTimer = 0.0f;
		}
	}

	if (m_isWaiting)
	{
		//待ち時間の間は速度ゼロ
		owner->m_rb.m_vel = Vector3(0, 0, 0);
		m_waitTimer += System::GetInstance().GetTimeScale();

		//waitTimeはこのポイントにたどり着いてから待つ時間
		//待機時間が終わったら次のパトロールポイントへ出発する//0ならすぐ出発
		//待機時間が負の値の場合はずっと待機させる
		if (patrolPoint.waitTime >= 0.0f && m_waitTimer >= patrolPoint.waitTime)
		{
			owner->m_currentPatrolIndex = (owner->m_currentPatrolIndex + 1) % patrolMaxCount;
			m_isWaiting = false;
			m_waitTimer = 0.0f;
		}
	}

	//速度があるならアニメーションを歩きにする
	//速度がないならアニメーションを待機にする
	//切り替わった瞬間だけChangeAnimを呼ぶ
	bool isWalking = owner->GetRigidBody().m_vel.Magnitude() > 0.1f;
	if (isWalking != m_wasWalking)
	{
		m_wasWalking = isWalking;
		const char* animKey = isWalking ? "Walk" : "Idle";
		owner->m_anim.ChangeAnim(owner->GetAnimName(animKey), true, 0.5f * owner->GetAnimSpeed(animKey));
	}


	//もしplayerが見えたら、追跡状態に移行する
	if (CanSeePlayer())
	{
		owner->m_isPlayerFound = true;
		owner->ChangeState(owner->NextOnPlayerFound());
	}
}

void EnemyPatrol::Exit()
{
	auto owner = m_owner.lock();
	if (!owner)return;
	//落下禁止を無効化
	owner->SetPreventFall(false);
}

void EnemyPatrol::DebugDraw()
{


	auto owner = m_owner.lock();
	if (!owner) return;

	const auto& points = owner->m_patrolPoints;
	int pointCount = static_cast<int>(points.size());
	for (int i = 0; i < pointCount; ++i)
	{
		//今向かっているポイントだけ色を変える
		unsigned int color = (i == owner->m_currentPatrolIndex) ? kDebugTargetPointColor : kDebugPointColor;
		DrawSphere3D(points[i].pos.ToDxLibVector(), kDebugPointRadius, kDebugPointDivNum, color, color, true);

		//次のポイントへの線(最後のポイントは最初のポイントへ戻る)
		const auto& next = points[(i + 1) % pointCount];
		DrawLine3D(points[i].pos.ToDxLibVector(), next.pos.ToDxLibVector(), kDebugRouteLineColor);
	}

	//視界(扇形)の描画
	Vector3 center = owner->GetRigidBody().GetPos();
	center.y += kDebugViewHeight;

	Vector3 forward = owner->GetTargetVec();
	forward.y = 0.0f;
	if (forward.Magnitude() <= 0.0001f) return;
	forward = forward.Normalize();

	const float viewLength = owner->m_status.viewLength;
	const float viewHalfAngle = owner->m_status.viewHalfAngleDeg * DX_PI_F / 180.0f;//度をラジアンに変換

	//プレイヤーが視界内なら色を変える
	unsigned int viewColor = CanSeePlayer() ? kDebugViewFoundColor : kDebugViewColor;

	//forwardをY軸まわりにangleだけ回した先の点を返す
	//ラムダ式
	//関数の中でしか使わないので、関数の中で定義する
	//可読性の向上、ヘッダーを触らずにすむためこの形
	auto getArcPoint = [&](float angle)
		{
			float c = cosf(angle);
			float s = sinf(angle);
			Vector3 dir(forward.x * c - forward.z * s, 0.0f, forward.x * s + forward.z * c);
			return center + dir * viewLength;
		};

	//左右の端の線
	DrawLine3D(center.ToDxLibVector(), getArcPoint(-viewHalfAngle).ToDxLibVector(), viewColor);
	DrawLine3D(center.ToDxLibVector(), getArcPoint(viewHalfAngle).ToDxLibVector(), viewColor);

	//円弧(細かい線分をつなげる)
	for (int i = 0; i < kDebugViewArcDivNum; ++i)
	{
		float a0 = -viewHalfAngle + (viewHalfAngle * 2.0f) * i / kDebugViewArcDivNum;
		float a1 = -viewHalfAngle + (viewHalfAngle * 2.0f) * (i + 1) / kDebugViewArcDivNum;
		DrawLine3D(getArcPoint(a0).ToDxLibVector(), getArcPoint(a1).ToDxLibVector(), viewColor);
	}
}

bool EnemyPatrol::IsInView(const Vector3& targetPos) const
{
	auto owner = m_owner.lock();
	if (!owner)return false;

	const float viewLength = owner->m_status.viewLength;
	const float viewHalfAngle = owner->m_status.viewHalfAngleDeg * DX_PI_F / 180.0f;//度→ラジアン

	//XZ平面での距離を計算
	Vector3 toTarget = targetPos - owner->GetRigidBody().GetPos();
	toTarget.y = 0.0f;

	float distance = toTarget.Magnitude();

	//距離が大きかったら視界外
	if (distance > viewLength)return false;

	//角度を指定
	Vector3 forward = owner->GetTargetVec();
	forward.y = 0.0f;

	float dot = forward.Normalize().Dot(toTarget.Normalize());

	//視野角以内なら視界内
	if(dot > cosf(viewHalfAngle))
	{
		return true;
	}


	return false;
}

bool EnemyPatrol::CanSeePlayer() const
{
	auto owner = m_owner.lock();
	if (!owner) return false;
	auto player = owner->m_player.lock();
	if (!player) return false;
	return IsInView(player->GetRigidBody().GetPos());
}
