#pragma once
#include "Math/Vector3.h"
//#include "../Managers/CollisionManager.h"
//#include "../IDManager.h"
#include "RigidBody.h"
#include <string>
#include <memory>

class Collider : public std::enable_shared_from_this<Collider>
{
public:
	/// <summary>
	/// 形状
	/// </summary>
	enum class Type
	{
		Sphere,
		Box,
		Capsule,
		Polygon
	};
	/// <summary>
	/// タグ
	/// </summary>
	enum class Tags
	{
		None = -1,
		StaticObject,//静的オブジェクト
		Player,
		PlayerHit,
		PlayerAttack,
		PlayerUltAttack,
		Enemy = 5,
		EnemyHit,
		EnemyAttack,
		Boss,
		WaveArea,
		Mascot
	};

	/// <summary>
	/// 位置補正の優先度
	/// </summary>
	enum class ColPriority
	{
		//優先度が低いほど、位置補正の処理が後になる
		None = -1,
		Low = 0,
		Middle,
		High,
		Static,
	};

	struct CapsuleInfo
	{
		//自身の座標とendPosの2点でカプセルを作る
		Vector3 endPos;
		//最近点
		Vector3 hitNearestPos;
		//最近点からの距離
		float hitNearestDistance;
	};

	struct BoxInfo
	{
		Vector3 halfExtents;//半分の大きさ//x,y,z
	};

public:
	Collider();
	virtual ~Collider();

	void ColUpdate();//当たり判定の更新//

protected:
	RigidBody m_rb;
	
	bool m_isActive;//当たり判定が有効かどうか
	bool m_isFloor;//床についているかどうか
	bool m_isWall;//壁にあたったかどうか
	int m_id = -1;//当たり判定などに使うID

	float m_lifeTime = 0.0f;//寿命
	bool m_isLifeTimeLimited = false;//trueになったらCollisionManagerから削除される
	float m_ownTimeScale = 1.0f;//自分のTimeScale
	float m_timeCounter = 0.0f;//TimeScaleのカウンター









};

