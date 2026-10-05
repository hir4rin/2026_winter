#include "CollisionManager.h"
#include "../Collider/Collider.h"
#include "../Collider/CapsuleShape.h"
#include"../System.h"
#include "../Stage/Stage.h"
#include <algorithm>
#include <cassert>

namespace
{
	constexpr float kMinFloorNormalY = 0.5f;//これ以上なら床
	constexpr float kGroundSnapDistance = 15.0f;//下り坂で離れる量の許容//playerの移動量に依存している
	constexpr float kSnapOverlapGap = 1.0f;//少し離す
}

bool CollisionManager::IsSameOwner(const std::weak_ptr<Collider>& a, const std::weak_ptr<Collider>& b)
{
	return !a.owner_before(b) && !b.owner_before(a);
}

bool CollisionManager::IsWallZone(const Collider& col)
{
	return col.GetRole() == Collider::ColRole::WallKickZone ||
		col.GetRole() == Collider::ColRole::WallRunZone;
}

void CollisionManager::RegisterCollider(std::weak_ptr<Collider> collider)
{
	//期限切れ(すでに実体が破棄されている)のweak_ptrは登録できない
	if (collider.expired())
	{
		assert(false && "RegisterColliderに無効なColliderが渡されました");
		return;
	}

	//すでに登録されているかどうかを確認する
	auto it = std::find_if(m_colliders.begin(), m_colliders.end(),
		[&collider](const std::weak_ptr<Collider>& registered)
		{
			return IsSameOwner(registered, collider);
		});

	if (it == m_colliders.end())
	{
		m_colliders.push_back(collider);
	}
	else
	{
		assert(false && "RegisterColliderが既に登録されています");
	}

}

void CollisionManager::ReleaseCollider(std::weak_ptr<Collider> collider)
{
	//登録されているものを探して、削除する
	auto it = std::find_if(m_colliders.begin(), m_colliders.end(),
		[&collider](const std::weak_ptr<Collider>& registered)
		{
			return IsSameOwner(registered, collider);
		});

	if (it != m_colliders.end())
	{
		m_colliders.erase(it);
	}
	else
	{
		//assert(false && "RemoveCOLLIDERが見つかりませんでした");
	}
}

void CollisionManager::Init()
{
	m_collisionChecker = std::make_unique<CollisionChecker>();
	m_fixNextPositioner = std::make_unique<FixNextPosition>();
}
void CollisionManager::SetStage(std::weak_ptr<Stage> stage)
{
	m_stage = stage;
}

void CollisionManager::Terminate()
{
	m_colliders.clear();
}

void CollisionManager::Update()
{
	//IsTriggerを作って、押し戻し判定を無視するという条件式を追加する//今は押し戻し判定を無視する条件式はない
	//PushBackの中でその処理をするのでいい

	//Stageのポインタをセットする//絶対この辺もっといい方法ある
	for (auto& weakCollider : m_colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider)continue;
		collider->SetStagePtr(m_stage);
	}

	//CollisionのUpdate(今は寿命カウント用)
	for (auto& weakCollider : m_colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider)continue;
		collider->ColUpdate();
	}



	//実体が破棄された、または寿命が尽きたコライダーを削除する
	std::erase_if(m_colliders, [](const std::weak_ptr<Collider>& weakCollider)
		{
			auto collider = weakCollider.lock();
			return !collider || collider->GetIsLifeTimeLimited();
		});


	//現在触れているコライダーのリストをクリアする
	for (auto& weakCollider : m_colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider) continue;
		collider->m_currentPressColliders.clear();
	}

	//速度を足す
	AddVelocity();

	//前フレームの接地状態を保存して、いったん空中扱いにする
	//押し戻しで床に当たればHitFloorCPがtrueに戻す
	for (auto& weakCollider : m_colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider)continue;
		//吸着スナップを行わないなら飛ばす
		if (!collider->m_useGroundSnap)continue;
		collider->m_wasFloor = collider->m_isFloor;
		collider->m_isFloor = false;
	}

	//OnCollision等の中でステート遷移→Register/ReleaseColliderされても壊れないようにコピーで回す
	auto colliders = m_colliders;
	for (int t = 0; t < 3; t++)
	{
		//すべてのコライダーの組み合わせをチェックする//当たっているかの確認かつ、速度をいじる
		for (size_t i = 0; i < colliders.size(); i++)
		{
			std::shared_ptr<Collider> colliderA = colliders[i].lock();
			if (!colliderA)continue;
			if (!colliderA->GetIsActive())continue;

			for (size_t j = i + 1; j < colliders.size(); j++)
			{

				std::shared_ptr<Collider> colliderB = colliders[j].lock();
				//アクティブなコライダーだけをチェックする//ここ関数化
				if (!colliderB)continue;
				if (!colliderB->GetIsActive())continue;
				//静的オブジェクト同士の時無視
				if (colliderA->GetTag().faction == Collider::Faction::StaticObject &&
					colliderB->GetTag().faction == Collider::Faction::StaticObject)continue;
				//壁ゾーンは押し戻し・衝突処理をしない(プレイヤーがStage::IsInWallZoneで問い合わせる)
				if (IsWallZone(*colliderA) || IsWallZone(*colliderB))continue;

				//衝突判定//球と球、BoxとBox、CapsuleとCapsuleとかで分ける
				if (m_collisionChecker->IsCollide(*colliderA, *colliderB))
				{
					//今触れているコライダーのリストに追加する
					colliderA->m_currentPressColliders.push_back(colliderB);
					colliderB->m_currentPressColliders.push_back(colliderA);

					//Trigger処理
					if (!ContainsCollider(colliderA->m_prevPressColliders, colliderB))
					{
						colliderA->OnTriggerEnter(*colliderB);
						colliderB->OnTriggerEnter(*colliderA);
					}


					//衝突したときの処理を呼び出す
					colliderA->OnCollision(*colliderB);
					colliderB->OnCollision(*colliderA);

					//ここで押し戻し
					//isTriggerは押し戻しを無視
					if (colliderA->GetIsTrigger() || colliderB->GetIsTrigger()) continue;
					//isGhostはキャラ同士の押し戻しを無視する
					if (colliderA->GetIsGhost() &&
						colliderB->GetFaction() != Collider::Faction::StaticObject)continue;
					if (colliderB->GetIsGhost() &&
						colliderA->GetFaction() != Collider::Faction::StaticObject)continue;


					//押し戻しの処理
					//ここで速度を変更する//ここでタイムスケールを変更<-？？多分違う
					//PushBackのvelを加える
					m_fixNextPositioner->FixNextPos(*colliderA, *colliderB);
				}
			}
		}
	}
	//ループが終わった後、ExitTriggerの処理を検出
	for (auto& weakCollider : colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider)continue;
		for (auto& weakPrev : collider->m_prevPressColliders)
		{
			auto prevCol = weakPrev.lock();
			if (!prevCol)continue;
			//当たっているコライダーに前フレームのコライダーが含まれていなければ、ExitTriggerの処理を呼ぶ
			if (!ContainsCollider(collider->m_currentPressColliders, prevCol))
			{
				//ExitTriggerの処理
				collider->OnTriggerExit(*prevCol);
				prevCol->OnTriggerExit(*collider);
			}
		}
		//更新
		collider->m_prevPressColliders = collider->m_currentPressColliders;
	}


	//床に当たらなかったものを真下の床に吸着させる
	SnapToGround();


	//ここで位置確定用の関数を読んで位置をおいておく
	//ここですべてのコライダーの位置を更新させる関数
	//速度をSetVelだと、どこからでもいじれちゃうけど、CollisionManagerがColのfriendクラスになって速度をいじれるようにして、更新させる
	//速度を足す
	ApplyAdjustments();
}


void CollisionManager::DebugDraw() const
{
	//登録されているすべてのコライダーのデバッグ描画を呼び出す
	for (const auto& weakCollider : m_colliders)
	{
		auto collider = weakCollider.lock();
		//アクティブなコライダーだけを描画する
		if (collider && collider->GetIsActive())
			collider->DebugDraw();
	}
}

std::shared_ptr<Collider> CollisionManager::GetColliderById(int id) const
{
	std::shared_ptr<Collider> ansCol;
	for (const auto& weakCollider : m_colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider)continue;
		if (collider->GetId() == id)
		{
			ansCol = collider;
			break;
		}
	}

	//見つからなかったらassert
	if (!ansCol)
	{
		assert(false && "GetColliderByIdで指定したidのコライダーが見つかりませんでした");
	}

	return ansCol;
}

bool CollisionManager::ContainsCollider(const std::vector<std::weak_ptr<Collider>>& list, const std::shared_ptr<Collider>& target)
{
	for (auto& weak : list)
	{
		std::shared_ptr<Collider> locked = weak.lock();

		//実体がtargetと同じかどうかを確認
		if (locked == target)
		{
			return true;
		}
	}

	//見つからなかった場合はfalseを返す
	return false;
}

void CollisionManager::ApplyAdjustments()
{
	//Colliderの座標を確定//Col自身に座標の更新をさせる
	//ApplyPos内でステート遷移→Register/ReleaseColliderされてもイテレータが壊れないようにコピーで回す
	auto colliders = m_colliders;
	for (auto& weakCollider : colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider)continue;
		if (!collider->GetIsActive())continue;
		//staticは無視
		if (collider->GetTag().faction == Collider::Faction::StaticObject)continue;

		collider->ApplyPos();
	}
}

void CollisionManager::AddVelocity()
{
	for (size_t i = 0; i < m_colliders.size(); i++)
	{
		std::shared_ptr<Collider> colliderA = m_colliders[i].lock();
		if (!colliderA)continue;
		if (!colliderA->GetIsActive())continue;
		float timescale = System::GetInstance().GetTimeScale();
		float ownScale = colliderA->m_ownTimeScale;
		//ここですべてのコライダーに速度、timescaleをかける
		colliderA->GetRigidBody().m_vel *= timescale * ownScale;
	}
}

void CollisionManager::SnapToGround()
{
	auto stage = m_stage.lock();
	if (!stage)return;

	for (auto& weakCollider : m_colliders)
	{
		auto collider = weakCollider.lock();
		if (!collider)continue;
		if (!collider->GetIsActive())continue;
		if (!collider->m_useGroundSnap)continue;
		//前フレーム地面にいて、押し戻しで床に当たらなかったものだけ
		if (!collider->m_wasFloor || collider->m_isFloor)continue;
		//上昇中(ジャンプ)は吸着しない
		if (collider->m_rb.m_vel.y > 0.0f)continue;

		auto capsule = dynamic_cast<CapsuleShape*>(&collider->GetShape());
		if (!capsule)continue;

		//カプセルの足がわが球の中心(押し戻し後の次の座標)
		Vector3 endPos = collider->GetNextPos();
		Vector3 startPos = endPos + capsule->GetEndPos();
		Vector3 legPos = (startPos.y < endPos.y) ? startPos : endPos;
		float radius = capsule->GetRadius();

		//坂の上でも届く長さ
		float rayLength = radius / kMinFloorNormalY + kGroundSnapDistance;
		Vector3 rayEnd = legPos + Vector3(0.0f, -rayLength, 0.0f);

		auto hit = MV1CollCheck_Line(stage->GetStageModelHandle(), -1,
		legPos.ToDxLibVector(), rayEnd.ToDxLibVector());
			
		//床がない→空中(m_isFloorはfalseのまま)
		if (!hit.HitFlag)continue;
		//壁ポリゴンは床扱いしない
		if (hit.Normal.y < kMinFloorNormalY)continue;

		//坂でもめり込まない高さ//normal.y = cosθ
		float targetY = hit.HitPosition.y + radius / hit.Normal.y + kSnapOverlapGap;

		//次の座標がtargetYになるように速度で補正する(ApplyPosで m_pos += m_vel される)
		collider->m_rb.m_vel.y += targetY - legPos.y;
		collider->m_isFloor = true;

	}
}

