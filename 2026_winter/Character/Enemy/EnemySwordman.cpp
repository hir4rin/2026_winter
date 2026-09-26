#include "EnemySwordman.h"
#include "../../DataLoader/DataManager.h"
#include "../System.h"
#include "../../Math/Matrix4x4.h"
#include "../../Game.h"
#include "../../Collider/SphereShape.h"
#include "../../Collider/CapsuleShape.h"
#include "HitCol.h"
#include "State/General/EnemyIdle.h"

namespace
{
	constexpr int kInitialHp = 100;//初期体力

	constexpr float kEnemyOffset = 40.0f;//カプセルの足のオフセット
	constexpr float kEnemyHead = 120.0f;//カプセルの頭の高さ
	constexpr float kRadius = 40.0f;//体の当たり判定の半径

	constexpr float kHitColOffsetY = 100.0f;//やられ判定の高さ
	constexpr float kRadiusHit = 50.0f;//やられ判定の半径
}


EnemySwordman::EnemySwordman(std::weak_ptr<Player> player, Vector3 startPos):EnemyBase(player)
{
	m_rb.m_pos = startPos;
	m_hp = kInitialHp;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyModel));

	//モデルの初期位置を設定する
	Matrix4x4 rotY = Matrix4x4::MakeRotationY(0);
	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);
	Matrix4x4 mtx = trans * rotY;
	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));

	//アニメーションの名前のマップの初期化
	const auto& animData = DataManager::GetInstance().GetEnemySwordmanAnimData();

	m_animNames = animData.animNames;
}

EnemySwordman::~EnemySwordman()
{
	if (m_currentState)
	{
		m_currentState->Exit();//状態を抜ける
	}
	MV1DeleteModel(m_modelHandle);
}

void EnemySwordman::Init()
{
	//初期状態をIdleにする//アニメーションの初期化
	m_anim.Init(m_modelHandle, GetAnimName("Idle"), true);

	//当たり判定の初期化
	ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, kEnemyOffset, 0),
		.shape = std::make_unique<CapsuleShape>(Vector3(0,kEnemyHead,0),kRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::None},
		.isActive = true
		});

	//地面への吸着を行う
	SetUseGroundSnap(true);

	//やられ判定の初期化
	InitHitCol(GetWeakPtr());
	m_hitCol->ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, kHitColOffsetY, 0),
		.shape = std::make_unique<SphereShape>(kRadiusHit),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::Hit},
		.isActive = true,
		.isTrigger = true
		});
	CharacterBase::ApplyPos();//座標の更新//モデルの座標を更新する
	ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));
}

void EnemySwordman::Update()
{
	//押し戻しの処理が続かないように消す//縦の速度(重力)は空中のステート(HitAir,HitDrop,AirFall)が自分で作る
	m_rb.m_vel = Vector3(0, 0, 0);

	if (m_currentState)
	{
		m_currentState->Update();//状態の更新
	}

	//アニメーションの更新
	m_anim.Update();
}

void EnemySwordman::Draw()
{
	MV1DrawModel(m_modelHandle);
#ifdef _DEBUG
	if (m_currentState)
	{
		m_currentState->DebugDraw();//デバッグ描画
	}
#endif
}

void EnemySwordman::OnCollision(Collider& other)
{
}

void EnemySwordman::OnDamage(Collider& other, AttackData& data)
{
	EnemyBase::OnDamage(other, data);
}
