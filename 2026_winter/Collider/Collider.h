#pragma once
#include "../Math/Vector3.h"
#include "../Managers/CollisionManager.h"
#include "../IDManager.h"
#include "RigidBody.h"
#include "ColliderSummery.h"
#include "ColliderShape.h"
#include <string>
#include <memory>
#include <vector>

class Stage;


class Collider : public std::enable_shared_from_this<Collider>
{
public:
	//陣営
	enum class Faction
	{
		None = -1,
		Player,
		Enemy,
		Boss,
		Mascot,
		StaticObject,
	};
	//役割
	enum class ColRole
	{
		None = -1,
		Hit,//当たり判定
		Attack,//攻撃判定
		UltAttack,//必殺技判定
		WallKickZone,//壁キックできるゾーン(ステージ編集で配置。中にいる間だけ壁キックできる)
		WallRunZone,//壁走りできるゾーン(ステージ編集で配置。中にいる間だけ壁走りできる)
		JustDodge,//ジャスト回避判定(回避中だけ存在する。敵の攻撃が当たったらジャスト回避成功)

	};

	//位置補正の優先度
	enum class ColPriority
	{
		//優先度が低いほど、位置補正の処理が後になる
		None = -1,
		Low = 0,
		Middle,
		High,
		Static,//固定
	};
	//タグ
	struct ColTag
	{
		Faction faction = Faction::None;//陣営
		ColRole role = ColRole::None;//役割
	};
	//Colliderの初期化パラメータ
	struct ColInitParam
	{
		Vector3 pos;
		Vector3 offset;
		std::unique_ptr<ColliderShape> shape;
		ColTag tag;
		bool isActive = true;
		bool isTrigger = false;
		float lifeTime = 0.0f;
	};
public:
	Collider();
	virtual ~Collider();

	//デバッグ描画
	void DebugDraw() const;

	virtual void OnCollision(Collider& other) = 0;//衝突時の処理

	virtual void OnTriggerEnter(Collider& other);//トリガー処理//当たり判定のみで、押し戻しなどはしない
	virtual void OnTriggerExit(Collider& other);//トリガーから離れたときの処理

	//Actorが派生先ですること-------------------------------------------------------------
	//当たり判定の初期化処理//登録
	/// <summary>座標、中心点、半径、当たり判定のタイプ、タグ、当たり判定が有効かどうか</summary>
	void  ColInit(ColInitParam);
	//IDのセット//子Colliderは別でidを持っているが親を持っているため、親のidも仕えるようにする設計にする
	void SetID();
	void SetStagePtr(std::weak_ptr<Stage> stage) { m_stage = stage; }//ステージへの弱参照をセット
	//--------------------------------------------------------------------------

	//ID・自身への参照
	std::shared_ptr<Collider> GetCollider() { return shared_from_this(); }
	int GetId()const { return m_id; }

	//RBのゲット
	RigidBody& GetRigidBody() { return m_rb; }

	//タイプ・タグ
	ColliderShape& GetShape()const  { return *m_shape; }
	ColliderType GetType() const { return m_shape->GetType(); }

	float GetRadius() const { return m_shape->GetRadius(); }
	Vector3 GetHalfExtents() const { return m_shape->GetHalfExtents(); }

	void SetTag(ColTag tag) { m_tag = tag; }
	ColTag GetTag()const { return m_tag; }
	Faction GetFaction()const { return m_tag.faction; }
	ColRole GetRole()const { return m_tag.role; }

	//有効・トリガー
	void SetIsActive(bool isActive) { m_isActive = isActive; }
	bool GetIsActive()const { return m_isActive; }
	bool GetIsTrigger()const { return m_isTrigger; }
	void SetIsGhost(bool isGhost) { m_isGhost = isGhost; }
	bool GetIsGhost()const { return m_isGhost; }

	//地面・壁への接触
	void SetIsFloor(bool isFloor) { m_isFloor = isFloor; }
	bool IsFloor()const { return m_isFloor; }
	void SetUseGroundSnap(bool use) { m_useGroundSnap = use;}//接地の吸着を行うかどうか//player、Enemyなどをtrue
	bool IsLeftFloor()const { return m_wasFloor && !m_isFloor; }//このフレームで地面から離れたか(歩いて落ちた時用)
	void SetIsWall(bool isWall) { m_isWall = isWall; }
	bool IsWall()const { return m_isWall; }

	//座標・速度
	Vector3 GetWorldPos() const { return m_rb.m_pos + m_offset; }//ワールド座標での中心位置を返す//当たり判定の中心位置
	Vector3 GetNextPos() const { return GetWorldPos() + m_rb.m_vel; }//次のフレームでの座標を返す
	//寿命
	void SetLifeTimeLimited() { m_isLifeTimeLimited = true; }
	bool GetIsLifeTimeLimited() const { return m_isLifeTimeLimited; }

	//タイムスケール
	void SetOwnTimeScale(float timeScale, float time) { m_ownTimeScale = timeScale; m_timeCounter = time; }
	float GetTimeScale() const { return m_ownTimeScale; }


	// シェイプを差し替える（Init時に使う）
	void SetShape(std::unique_ptr<ColliderShape> shape) { m_shape = std::move(shape); }

	void ColUpdate();//当たり判定の更新//

protected:
	//座標の更新
	virtual void ApplyPos() = 0;
protected:
	std::unique_ptr<ColliderShape> m_shape;//当たり判定の形状
	RigidBody m_rb;//剛体
	Vector3 m_offset;//当たり判定の中心からのオフセット

	ColTag m_tag;//陣営と役割
	
	bool m_isActive = false;;//当たり判定が有効かどうか
	bool m_isTrigger = false;;//押しもどしを行わない当たり判定かどうか
	bool m_isGhost = false;//キャラ同士の押し戻しだけ無視する

	bool m_isFloor = true;;//床についているかどうか
	bool m_wasFloor = true;//押し戻し前に床についていたかどうか
	bool m_useGroundSnap = false;//地面の吸着を行うかどうか

	bool m_isWall = false;//壁にあたったかどうか
	int m_id = -1;//当たり判定などに使うID

	float m_lifeTime = 0.0f;//寿命
	bool m_isLifeTimeLimited = false;//trueになったらCollisionManagerから削除される
	float m_ownTimeScale = 1.0f;//自分のTimeScale
	float m_timeCounter = 0.0f;//TimeScaleのカウンター

	std::weak_ptr<Stage> m_stage;//ステージへの弱参照

	std::vector<std::weak_ptr<Collider>> m_currentPressColliders;///現在触れているコライダーのリスト
	std::vector<std::weak_ptr<Collider>> m_prevPressColliders;///前のフレームで触れていたコライダーのリスト


	
	friend class CollisionManager;
	friend class CollisionChecker;
	friend class FixNextPosition;

};

