#include "Collider.h"
#include "../System.h"

Collider::Collider()
{
}

Collider::~Collider()
{
}

void Collider::DebugDraw() const
{
	if (!m_isActive) return;
	unsigned int color = GetColor(0, 0, 0);
	switch (m_tag.faction)
	{
	case Faction::Player:
		color = GetColor(0, 255, 0);//プレイヤーは緑
		break;
	case Faction::Enemy:
		color = GetColor(128, 128, 128);//敵は灰色
		break;
	case Faction::StaticObject:
		color = GetColor(255, 255, 0);//静的オブジェクト(ステージ制作モードのBOXなど)は黄色
		break;
	default:
		break;
	}
	//壁ゾーンは役割で色を分ける
	if (m_tag.role == ColRole::WallKickZone)
	{
		color = GetColor(255, 128, 0);//壁キックゾーンはオレンジ
	}
	else if (m_tag.role == ColRole::WallRunZone)
	{
		color = GetColor(0, 200, 255);//壁走りゾーンは水色
	}
	else if (m_tag.role == ColRole::JustDodge)
	{
		color = GetColor(255, 0, 255);//ジャスト回避判定はマゼンタ
	}

	m_shape->DebugDraw(GetWorldPos(), color); // switch(m_type)が丸ごと消える
}

void Collider::OnTriggerEnter(Collider& other)
{
}

void Collider::OnTriggerExit(Collider& other)
{
}

void Collider::ColInit(ColInitParam param)
{
	m_rb.m_pos = param.pos;
	m_offset = param.offset;
	m_shape = std::move(param.shape);
	m_tag = param.tag;

	m_isActive = param.isActive;
	m_isTrigger = param.isTrigger;
	m_lifeTime = param.lifeTime;
	SetID();

	//コライダーをコリジョンマネージャーに登録する
	CollisionManager::GetInstance().RegisterCollider(shared_from_this());
}

void Collider::SetID()
{
	m_id = IDManager::GetNextID();
}




void Collider::ColUpdate()
{
	//自身のタイムスケールの計算
	if (m_ownTimeScale != 1.0f)
	{
		m_timeCounter -= 1.0f * System::GetInstance().GetTimeScale();
		if (m_timeCounter <= 0.0f)
		{
			m_ownTimeScale = 1.0f;
		}
	}
	//寿命計算
	float timeScale = System::GetInstance().GetTimeScale();
	if (m_lifeTime > 0.0f)
	{
		m_lifeTime -= 1.0f * timeScale * m_ownTimeScale;

		if (m_lifeTime <= 0.0f)
		{
			m_isActive = false;
			m_isLifeTimeLimited = true;
		}
	}
}
