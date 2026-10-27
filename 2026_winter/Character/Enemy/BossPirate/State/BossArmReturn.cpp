#include "BossArmReturn.h"
#include "Character/Enemy/BossPirate/Part/BossArm.h"
#include "Main/System.h"
#include "Math/Easing.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kReturnFrame = 30.0f;//戻るまで(F)
	constexpr float kReturnEasingPower = 2.0f;
	constexpr float kOneTurnDeg = 360.0f;
}

BossArmReturn::BossArmReturn(std::weak_ptr<EnemyBase> owner) :
	EnemyStateBase(owner)
{
}

BossArmReturn::~BossArmReturn()
{
}

void BossArmReturn::Enter()
{
	auto arm = std::dynamic_pointer_cast<BossArm>(m_owner.lock());
	if (!arm)return;
	m_timer = 0.0f;
	m_startMove = arm->m_detachMove;
	m_startSpinDeg = arm->m_detachSpinDeg;
	//回った分を戻しすぎないように、一番近い一周分へ//roundfは今の角度に一番近い360の倍数を出すらしい//ようは最短経路
	m_goalSpinDeg = roundf(m_startSpinDeg / kOneTurnDeg) * kOneTurnDeg;
	arm->PlayAnim("Idle", true);
}

void BossArmReturn::Update()
{
	auto arm = std::dynamic_pointer_cast<BossArm>(m_owner.lock());
	if (!arm)return;

	m_timer += System::GetInstance().GetTimeScale() * arm->m_ownTimeScale;
	const float t = (std::min)(m_timer / kReturnFrame, 1.0f);//windows.hのminマクロとぶつからないように()で囲む
	const float rate = Easing::Apply(EasingMode::EaseInOut, t, kReturnEasingPower);

	//回転とずれを0に戻しながら、ボスに付いた形へ寄せる
	arm->m_detachMove = m_startMove * (1.0f - rate);
	arm->m_detachSpinDeg = m_startSpinDeg + (m_goalSpinDeg - m_startSpinDeg) * rate;
	arm->m_returnRate = rate;

	//着いた
	if (t >= 1.0f)
	{
		arm->ReturnToIdle();
		return;
	}
}

void BossArmReturn::Exit()
{
}
