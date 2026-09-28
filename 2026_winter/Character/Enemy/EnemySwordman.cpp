#include "EnemySwordman.h"
#include "../../DataLoader/DataManager.h"
#include "../System.h"
#include "../../Math/Matrix4x4.h"
#include "../../Collider/SphereShape.h"
#include "../../Collider/CapsuleShape.h"
#include "EnemyPart.h"
#include "HitCol.h"
#include "State/General/EnemyIdle.h"
#include "../Input.h"

namespace
{
	constexpr int kInitialHp = 100;//初期体力

	constexpr float kEnemyOffset = 40.0f;//カプセルの足のオフセット
	constexpr float kEnemyHead = 120.0f;//カプセルの頭の高さ
	constexpr float kRadius = 40.0f;//体の当たり判定の半径

	constexpr float kHitColOffsetY = 100.0f;//やられ判定の高さ
	constexpr float kRadiusHit = 50.0f;//やられ判定の半径

	constexpr int kLeftArmFrame = 21;//左腕のボーン(mixamorig:LeftArm)
	constexpr int kHeadFrame = 46;//頭のボーン(mixamorig:Head)
	constexpr float kBreakPartScale = 0.001f;//部位破壊したボーンの大きさ//0だと法線が壊れるので少しだけ残す

	constexpr float kPartModelScale = 0.01f;//左腕モデルと本体モデルの単位スケール差の補正値//大きさが合わなければここを調整する
	constexpr float kHeadModelScale = 0.01f;//頭モデルと本体モデルの単位スケール差の補正値//大きさが合わなければここを調整する
	constexpr float kPartColliderRadius = 15.0f;//切り離したパーツの当たり判定の半径

	constexpr float kPartJumpUpSpeed = 15.0f;//切り離した瞬間の上向きの速度
	constexpr float kPartJumpSideSpeed = 6.0f;//切り離した瞬間の横方向(敵の右側)の速度

}


EnemySwordman::EnemySwordman(std::weak_ptr<Player> player, Vector3 startPos):EnemyBase(player)
{
	m_rb.m_pos = startPos;
	m_hp = kInitialHp;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyModel));
	//部位破壊のパーツのモデル//読み込みはSystemで済ませているので複製する
	m_leftArmModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyPartLeftArmModel));
	m_headModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyPartHeadModel));

	m_leftArmPart = std::make_shared<EnemyPart>(m_leftArmModelHandle);
	m_headPart = std::make_shared<EnemyPart>(m_headModelHandle);

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
	MV1DeleteModel(m_leftArmModelHandle);
	MV1DeleteModel(m_headModelHandle);
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

	//部位破壊で落ちる左腕の当たり判定//切り離すまでは非アクティブにしておく
	m_leftArmPart->ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(),
		.shape = std::make_unique<SphereShape>(kPartColliderRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::None},
		.isActive = false
		});
	m_leftArmPart->SetIsGhost(true);//キャラクター同士の押し戻しは無視する(地面・壁とはそのまま当たる)
	m_leftArmPart->SetUseGroundSnap(true);//地面についたかどうかの判定をCollisionManagerに毎フレームリセットしてもらうため

	//部位破壊で落ちる頭の当たり判定//切り離すまでは非アクティブにしておく
	m_headPart->ColInit({
		.pos = m_rb.m_pos,
		.shape = std::make_unique<SphereShape>(kPartColliderRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::None},
		.isActive = false
		});
	m_headPart->SetIsGhost(true);
	m_headPart->SetUseGroundSnap(true);
}

void EnemySwordman::Update()
{
	//押し戻しの処理が続かないように消す//縦の速度(重力)は空中のステート(HitAir,HitDrop,AirFall)が自分で作る
	m_rb.m_vel = Vector3(0, 0, 0);
	auto& input = Input::GetInstance();

	//デバッグ用//Zで左腕を部位破壊する
	if (CheckHitKey(KEY_INPUT_Z) && !m_isBreakLeftArm)
	{
		SetUpBreakLeftArm();
	}

	//デバッグ用//Xで頭を部位破壊する
	if (CheckHitKey(KEY_INPUT_X) && !m_isBreakHead)
	{
		SetUpBreakHead();
	
	}

	if (m_isBreakLeftArm)
	{
		m_leftArmPart->Update();
	}

	if (m_isBreakHead)
	{
		m_headPart->Update();
	}

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
	//部位破壊したパーツの描画
	if (m_isBreakLeftArm) m_leftArmPart->Draw();
	if (m_isBreakHead) m_headPart->Draw();
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

void EnemySwordman::OnAssasined()
{
	EnemyBase::OnAssasined();
}

void EnemySwordman::OnHeadBreak()
{
	SetUpBreakHead();
	//SetUpBreakLeftArm();
}

void EnemySwordman::SetUpBreakLeftArm()
{
	//切り落とした腕のモデルを、本体の向きのまま左腕ボーンの付け根に置く
	MATRIX partMat = MV1GetMatrix(m_modelHandle);
	//パーツモデルの単位スケール差を補正する(回転・拡縮成分のみ)
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			partMat.m[i][j] *= kPartModelScale;
		}
	}
	VECTOR bonePos = MV1GetFramePosition(m_modelHandle, kLeftArmFrame);
	//上方向＋敵の右方向に飛び上がらせる
	Vector3 initialVel = Vector3(0, kPartJumpUpSpeed, 0) + GetForward().Cross(Vector3(0, 1, 0)) * kPartJumpSideSpeed;
	m_leftArmPart->Break(partMat, Vector3::FromDxLibVector(bonePos), initialVel);

	//本体の左腕はボーンを縮めて消す
	MATRIX scaleMat = MMult(MGetScale(VGet(kBreakPartScale, kBreakPartScale, kBreakPartScale)),
		MV1GetFrameBaseLocalMatrix(m_modelHandle, kLeftArmFrame));
	MV1SetFrameUserLocalMatrix(m_modelHandle, kLeftArmFrame, scaleMat);
	m_isBreakLeftArm = true;
}

void EnemySwordman::SetUpBreakHead()
{
	//切り落とした頭のモデルを、本体の向きのまま頭ボーンの付け根に置く
	MATRIX partMat = MV1GetMatrix(m_modelHandle);
	//パーツモデルの単位スケール差を補正する(回転・拡縮成分のみ)
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			partMat.m[i][j] *= kHeadModelScale;
		}
	}
	VECTOR bonePos = MV1GetFramePosition(m_modelHandle, kHeadFrame);
	//上方向＋敵の右方向に飛び上がらせる
	Vector3 initialVel = Vector3(0, kPartJumpUpSpeed, 0) + GetForward().Cross(Vector3(0, 1, 0)) * kPartJumpSideSpeed;
	m_headPart->Break(partMat, Vector3::FromDxLibVector(bonePos), initialVel);

	//本体の頭はボーンを縮めて消す
	MATRIX scaleMat = MMult(MGetScale(VGet(kBreakPartScale, kBreakPartScale, kBreakPartScale)),
		MV1GetFrameBaseLocalMatrix(m_modelHandle, kHeadFrame));
	MV1SetFrameUserLocalMatrix(m_modelHandle, kHeadFrame, scaleMat);
	m_isBreakHead = true;
}
