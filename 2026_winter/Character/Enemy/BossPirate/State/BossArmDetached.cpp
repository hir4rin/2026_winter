#include "BossArmDetached.h"
#include "Character/Enemy/BossPirate/Part/BossArm.h"
#include "Main/System.h"
#include "Math/Easing.h"
#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kRiseFrame = 30.0f;//上がりきるまで(F)
	constexpr float kRiseHeight = 300.0f;//上がる高さ
	constexpr float kRiseEasingPower = 2.0f;
	constexpr float kSpinSpeedDeg = 12.0f;//最初の回る速さ(度/F)//上がりきると止まる
	constexpr float kFloatSpeed = 0.08f;//浮いている間の上下の速さ
	constexpr float kFloatWidth = 15.0f;//浮いている間の上下の幅
}

BossArmDetached::BossArmDetached(std::weak_ptr<EnemyBase> owner) :
	EnemyStateBase(owner)
{
}

BossArmDetached::~BossArmDetached()
{
}

void BossArmDetached::Enter()
{
	auto arm = std::dynamic_pointer_cast<BossArm>(m_owner.lock());
	if (!arm)return;
	m_timer = 0.0f;
	arm->PlayAnim("BlockBreak", false);
}

void BossArmDetached::Update()
{
	auto arm = std::dynamic_pointer_cast<BossArm>(m_owner.lock());
	if (!arm)return;

	const float frame = System::GetInstance().GetTimeScale() * arm->m_ownTimeScale;//このフレームで進む量
	m_timer += frame;

	//上がる
	const float t = (std::min)(m_timer / kRiseFrame, 1.0f);//windows.hのminマクロとぶつからないように()で囲む
	const float rate = Easing::Apply(EasingMode::EaseOut, t, kRiseEasingPower);
	float height = kRiseHeight * rate;
	//上がりきったらふわふわ
	if (t >= 1.0f)height += sinf((m_timer - kRiseFrame) * kFloatSpeed) * kFloatWidth;
	arm->m_detachMove = Vector3(0.0f, height, 0.0f);

	//回転はだんだん止まる
	arm->m_detachSpinDeg += kSpinSpeedDeg * (1.0f - rate) * frame;
}

void BossArmDetached::Exit()
{
}
