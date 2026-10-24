#include "EnemyBazooka.h"
#include "Character/Enemy/Bazooka/Part/EnemyBazookaWeapon.h"
#include "DataLoader/DataManager.h"
#include "Main/System.h"
#include "Math/Matrix4x4.h"
#include "Collision/SphereShape.h"
#include "Collision/CapsuleShape.h"
#include "Character/Enemy/EnemyPart.h"
#include "Collision/HitCol.h"
#include "Character/Enemy/State/Locomotion/EnemyIdle.h"
#include "Render/ModelShaderManager.h"

namespace
{
	const std::string kStatusType = "Bazooka";//EnemyStatus.csvのtype

	constexpr float kBreakPartScale = 0.001f;//部位破壊したボーンの大きさ//0だと法線が壊れるので少しだけ残す

	constexpr float kPartColliderRadius = 15.0f;//切り離したパーツの当たり判定の半径

	constexpr float kPartJumpUpSpeed = 15.0f;//切り離した瞬間の上向きの速度
	constexpr float kPartJumpSideSpeed = 6.0f;//切り離した瞬間の横方向(敵の右側)の速度
}


EnemyBazooka::EnemyBazooka(std::weak_ptr<Player> player, Vector3 startPos):EnemyBase(player)
{
	//ステータス
	InitializeStatus(DataManager::GetInstance().GetEnemyStatusRawData(), kStatusType);
	//攻撃のデータ
	InitializeAttackList(DataManager::GetInstance().GetEnemyBazookaAttackRawData());

	m_rb.m_pos = startPos;
	m_hp = m_status.hp;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyGunnerModel));//体はガンナーと同じモデル
	m_waistFrame = MV1SearchFrame(m_modelHandle, "mixamorig:Hips");//腰のボーン
	//部位破壊のパーツのモデル//読み込みはSystemで済ませているので複製する//頭はソードマンと同じもの
	m_leftArmModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyGunnerPartLeftArmModel));//左腕はガンナーと同じ盾なしのもの
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
	const auto& animData = DataManager::GetInstance().GetEnemyBazookaAnimData();

	m_animNames = animData.animNames;
	m_animSpeeds = animData.animSpeeds;//アニメCSVの再生速度
}

EnemyBazooka::~EnemyBazooka()
{
	if (m_currentState)
	{
		m_currentState->Exit();//状態を抜ける
	}
	MV1DeleteModel(m_modelHandle);
	MV1DeleteModel(m_leftArmModelHandle);
	MV1DeleteModel(m_headModelHandle);
}

void EnemyBazooka::Init()
{
	//初期状態をIdleにする//アニメーションの初期化
	m_anim.Init(m_modelHandle, GetAnimName("Idle"), true, GetAnimSpeed("Idle"));

	//当たり判定の初期化
	ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, m_status.bodyColOffsetY, 0),
		.shape = std::make_unique<CapsuleShape>(Vector3(0,m_status.bodyColHeight,0),m_status.bodyColRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::None},
		.isActive = true
		});

	//地面への吸着を行う
	SetUseGroundSnap(true);

	//やられ判定の初期化
	InitHitCol(GetWeakPtr());
	m_hitCol->ColInit({
		.pos = m_rb.m_pos,
		.offset = Vector3(0, m_status.hitColOffsetY, 0),
		.shape = std::make_unique<SphereShape>(m_status.hitColRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::Hit},
		.isActive = true,
		.isTrigger = true
		});
	CharacterBase::ApplyPos();//座標の更新//モデルの座標を更新する
	ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));

	m_weapon = std::make_shared<EnemyBazookaWeapon>(std::static_pointer_cast<EnemyBazooka>(GetSharedPtr()));//武器の生成

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

void EnemyBazooka::Update()
{
	//押し戻しの処理が続かないように消す//縦の速度(重力)は空中のステート(HitAir,HitDrop,AirFall)が自分で作る
	m_rb.m_vel = Vector3(0, 0, 0);

	//デバッグ用//Zで左腕を部位破壊する
	if (CheckHitKey(KEY_INPUT_Z) && !m_isBreakLeftArm)
	{
		SetUpBreakLeftArm();
		m_isPartBroken = true;
	}

	//デバッグ用//Xで頭を部位破壊する
	if (CheckHitKey(KEY_INPUT_X) && !m_isBreakHead)
	{
		SetUpBreakHead();
		m_isPartBroken = true;
	}

	//デバッグ用//Qを押した時だけEnemyBazookaAttack.csvを読み込み直して攻撃データを更新する
	bool isQPressed = CheckHitKey(KEY_INPUT_Q) != 0;
	if (isQPressed && !m_wasQPressed)
	{
		DataManager::GetInstance().ReloadEnemyBazookaAttackRawData();
		InitializeAttackList(DataManager::GetInstance().GetEnemyBazookaAttackRawData());
		DataManager::GetInstance().ReloadEnemyStatusRawData();
		InitializeStatus(DataManager::GetInstance().GetEnemyStatusRawData(), kStatusType);
	}
	m_wasQPressed = isQPressed;

	if (m_isBreakLeftArm)
	{
		m_leftArmPart->Update();
	}

	if (m_isBreakHead)
	{
		m_headPart->Update();
	}

	//攻撃のクールタイムを減らす
	if (m_attackCoolTime > 0.0f)
	{
		m_attackCoolTime -= System::GetInstance().GetTimeScale() * m_ownTimeScale;
	}

	//しばらく被弾しなければ確定ヒットを解除
	m_forceHitTimer += System::GetInstance().GetTimeScale() * m_ownTimeScale;
	if (m_forceHitTimer > m_status.forceHitResetTime)
	{
		m_forceHitCount = 0;
	}

	if (m_currentState)
	{
		m_currentState->Update();//状態の更新
	}
	//アニメーションの更新//ここで1回だけ行う(各Stateでは更新しない)
	m_anim.Update(m_ownTimeScale);
	//頭をプレイヤーの方に向ける//アニメーションの姿勢に回転を足すので、アニメーションの更新の後に呼ぶ
	UpdateLookAtPlayer(m_status.headFrame);
	//武器の更新//手のボーンに合わせるので、頭の向きまで決まった後に呼ぶ
	m_weapon->Update();
}

void EnemyBazooka::Draw()
{
	m_weapon->Draw();//武器
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);//血殺中は黒く描く
	//部位破壊したパーツの描画
	if (m_isBreakLeftArm) m_leftArmPart->Draw();
	if (m_isBreakHead) m_headPart->Draw();
}


void EnemyBazooka::OnCollision(Collider& other)
{
}

void EnemyBazooka::OnDamage(Collider& other, AttackData& data)
{
	EnemyBase::OnDamage(other, data);
}

void EnemyBazooka::OnAssasined()
{
	EnemyBase::OnAssasined();
}

void EnemyBazooka::OnHeadBreak()
{
	m_isPartBroken = true;
	SetUpBreakHead();
}

void EnemyBazooka::OnPartBreak()
{
	if (m_isBreakLeftArm)return;
	m_isPartBroken = true;
	SetUpBreakLeftArm();
}

void EnemyBazooka::SetOpacity(float rate)
{
	EnemyBase::SetOpacity(rate);
	if (m_weapon) m_weapon->SetOpacity(rate);
	MV1SetOpacityRate(m_leftArmModelHandle, rate);
	MV1SetOpacityRate(m_headModelHandle, rate);
}

void EnemyBazooka::SetUpBreakLeftArm()
{
	//切り落とした腕のモデルを、本体の向きのまま左腕ボーンの付け根に置く
	MATRIX partMat = MV1GetMatrix(m_modelHandle);
	//パーツモデルの単位スケール差を補正する(回転・拡縮成分のみ)
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			partMat.m[i][j] *= m_status.leftArmModelScale;
		}
	}
	VECTOR bonePos = MV1GetFramePosition(m_modelHandle, m_status.leftArmFrame);
	//上方向＋敵の右方向に飛び上がらせる
	Vector3 initialVel = Vector3(0, kPartJumpUpSpeed, 0) + GetForward().Cross(Vector3(0, 1, 0)) * kPartJumpSideSpeed;
	m_leftArmPart->Break(partMat, Vector3::FromDxLibVector(bonePos), initialVel);

	//本体の左腕はボーンを縮めて消す
	MATRIX scaleMat = MMult(MGetScale(VGet(kBreakPartScale, kBreakPartScale, kBreakPartScale)),
		MV1GetFrameBaseLocalMatrix(m_modelHandle, m_status.leftArmFrame));
	MV1SetFrameUserLocalMatrix(m_modelHandle, m_status.leftArmFrame, scaleMat);
	m_isBreakLeftArm = true;
}

void EnemyBazooka::SetUpBreakHead()
{
	//切り落とした頭のモデルを、本体の向きのまま頭ボーンの付け根に置く
	MATRIX partMat = MV1GetMatrix(m_modelHandle);
	//パーツモデルの単位スケール差を補正する(回転・拡縮成分のみ)
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			partMat.m[i][j] *= m_status.headModelScale;
		}
	}
	VECTOR bonePos = MV1GetFramePosition(m_modelHandle, m_status.headFrame);
	//上方向＋敵の右方向に飛び上がらせる
	Vector3 initialVel = Vector3(0, kPartJumpUpSpeed, 0) + GetForward().Cross(Vector3(0, 1, 0)) * kPartJumpSideSpeed;
	m_headPart->Break(partMat, Vector3::FromDxLibVector(bonePos), initialVel);

	//本体の頭はボーンを縮めて消す
	MATRIX scaleMat = MMult(MGetScale(VGet(kBreakPartScale, kBreakPartScale, kBreakPartScale)),
		MV1GetFrameBaseLocalMatrix(m_modelHandle, m_status.headFrame));
	MV1SetFrameUserLocalMatrix(m_modelHandle, m_status.headFrame, scaleMat);
	m_isBreakHead = true;
}
