#include "EnemyVanish.h"
#include "../../EnemyBase.h"
#include "../System.h"
#include <algorithm>

namespace
{
	constexpr float kVanishFrame = 60.0f;//透明になりきるまでのフレーム数
}

EnemyVanish::EnemyVanish(std::weak_ptr<EnemyBase> owner) :EnemyStateBase(owner)
{
}

EnemyVanish::~EnemyVanish()
{
}

void EnemyVanish::Enter()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	//アニメーションは変えない(死んだときのポーズのまま消す)
	owner->m_rb.m_vel = Vector3(0, 0, 0);
	//消えている途中の死体にプレイヤーが押し戻されないようにする
	owner->SetIsGhost(true);
	owner->m_isVanishing = true;
}

void EnemyVanish::Update()
{
	auto owner = m_owner.lock();
	if (!owner)return;

	owner->m_rb.m_vel = Vector3(0, 0, 0);

	m_timer += 1.0f * System::GetInstance().GetTimeScale();
	float rate = 1.0f - std::clamp(m_timer / kVanishFrame, 0.0f, 1.0f);
	//本体と、切り離したパーツをまとめて透明にする
	owner->SetOpacity(rate);

	//透明になりきったら、EnemyManagerに削除してもらう
	if (rate <= 0.0f)
	{
		owner->m_isVanished = true;
	}
}

void EnemyVanish::Exit()
{
}

void EnemyVanish::DebugDraw()
{
	DrawFormatString(10, 30, GetColor(255, 255, 255), "EnemyState:Vanish");
}
