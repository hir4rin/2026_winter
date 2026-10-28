#include "BossEnemyKingmech.h"
#include "DataLoader/DataManager.h"
#include "Main/System.h"
#include "Math/Matrix4x4.h"
#include "Collision/CapsuleShape.h"
#include "Collision/HitCol.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/State/Locomotion/EnemyIdle.h"
#include "Main/Input.h"
#include "Render/ModelShaderManager.h"
#include "Battle/BattleManager.h"
#include "Battle/AttackTokenManager.h"



namespace
{
	const std::string kStatusType = "BossEnemyKingmech";//EnemyStatus.csvのtype

	constexpr float kModelScale = 2.3f;//モデルの大きさ//Tポーズで高さ約200なので約460


	constexpr float kToTargetPower = 3.0f;//プレイヤーの正面に行くようにknockBackする力


	constexpr float kEnemyDistance = 50.0f;

	constexpr int kNormalAttackIndex = 0;//横斬り
	constexpr int kLargeRangedAttackIndex = 5;
	constexpr int kComboAttackIndex = 3;//コンボの1撃目
	constexpr int kSweepAttackIndex = 2;//周囲薙ぎ払い
	constexpr int kRushAttackIndex = 6;//とびかかり(突進)

	constexpr int kLargeAttackInterval = 10;//この回数攻撃したら全体攻撃
	constexpr float kBackDot = -0.5f;//正面との内積がこれより小さければ背後(120度より後ろ)
	constexpr float kRushMinDistance = 400.0f;//この距離より遠ければ突進
	constexpr float kRushMaxDistance = 1000.0f;//突進を出す最大の距離

}


BossEnemyKingmech::BossEnemyKingmech(std::weak_ptr<Player> player, Vector3 startPos):EnemyBase(player)
{

	//ステータス
	InitializeStatus(DataManager::GetInstance().GetEnemyStatusRawData(), kStatusType);
	//攻撃のデータ
	InitializeAttackList(DataManager::GetInstance().GetBossEnemyKingmechAttackRawData());

	m_rb.m_pos = startPos;
	m_hp = m_status.hp;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::KingmechModel));
	m_waistFrame = MV1SearchFrame(m_modelHandle, "pelvis");//腰のボーン
	m_modelScale = kModelScale;

	//モデルの初期位置を設定する
	Matrix4x4 rotY = Matrix4x4::MakeRotationY(0);
	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);
	Matrix4x4 scale = Matrix4x4::FromDxLibMatrix(MGetScale(VGet(kModelScale, kModelScale, kModelScale)));
	Matrix4x4 mtx = trans * rotY * scale;
	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));

	//アニメーションの名前のマップの初期化
	const auto& animData = DataManager::GetInstance().GetBossEnemyKingmechAnimData();

	m_animNames = animData.animNames;
	m_animSpeeds = animData.animSpeeds;//アニメCSVの再生速度
}

BossEnemyKingmech::~BossEnemyKingmech()
{
	if (m_currentState)
	{
		m_currentState->Exit();//状態を抜ける
	}
	MV1DeleteModel(m_modelHandle);
}

void BossEnemyKingmech::Init()
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
		.offset = Vector3(0, m_status.bodyColOffsetY, 0),
		.shape = std::make_unique<CapsuleShape>(Vector3(0,m_status.bodyColHeight,0),m_status.bodyColRadius),
		.tag = {Collider::Faction::Enemy, Collider::ColRole::Hit},
		.isActive = true,
		.isTrigger = true
		});
	CharacterBase::ApplyPos();//座標の更新//モデルの座標を更新する
	ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));
}

void BossEnemyKingmech::Update()
{
	//押し戻しの処理が続かないように消す//縦の速度(重力)は空中のステート(HitAir,HitDrop,AirFall)が自分で作る
	m_rb.m_vel = Vector3(0, 0, 0);

	//デバッグ用//Qを押した瞬間だけBossEnemyKingmechAttack.csvを読み込み直して攻撃データを更新する
	bool isQPressed = CheckHitKey(KEY_INPUT_Q) != 0;
	if (isQPressed && !m_wasQPressed)
	{
		DataManager::GetInstance().ReloadBossEnemyKingmechAttackRawData();
		InitializeAttackList(DataManager::GetInstance().GetBossEnemyKingmechAttackRawData());
		DataManager::GetInstance().ReloadEnemyStatusRawData();
		InitializeStatus(DataManager::GetInstance().GetEnemyStatusRawData(), kStatusType);
	}
	m_wasQPressed = isQPressed;

	//デバッグ用//Iを押した瞬間だけBossEnemyKingmechAnim.csvを読み込み直す//次にアニメが切り替わったときから効く
	bool isIPressed = CheckHitKey(KEY_INPUT_I) != 0;
	if (isIPressed && !m_wasIPressed)
	{
		DataManager::GetInstance().ReloadBossEnemyKingmechAnimData();
		const auto& animData = DataManager::GetInstance().GetBossEnemyKingmechAnimData();
		m_animNames = animData.animNames;
		m_animSpeeds = animData.animSpeeds;
	}
	m_wasIPressed = isIPressed;

	//攻撃のクールタイムを減らす
	if (m_attackCoolTime > 0.0f)
	{
		m_attackCoolTime -= System::GetInstance().GetTimeScale() * m_ownTimeScale;
	}

	//もし被弾中で攻撃のクールタイムが上がったら攻撃に移行
	if (std::dynamic_pointer_cast<EnemyHitGround>(m_currentState))
	{
		if (m_attackCoolTime <= 0.0f)
		{
			const int attackIndex = SelectAttackIndex();
			//攻撃できて攻撃権が取れたら予備動作へ
			auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock();
			if (attackIndex >= 0 && tokenMgr && tokenMgr->Request(*this))
			{
				m_currentAttackIndex = attackIndex;
				ChangeState(std::make_shared<EnemyAttackPre>(GetWeakPtr()));
				return;
			}
		}
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
}

void BossEnemyKingmech::Draw()
{
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);//血殺中は黒く描く
}


void BossEnemyKingmech::OnCollision(Collider& other)
{
}

int BossEnemyKingmech::SelectAttackIndex()
{
	//クールタイム中は攻撃しない
	if (m_attackCoolTime > 0.0f)return -1;

	//プレイヤーとの距離(XZ)
	Vector3 toPlayer = TargetPlayerPos() - m_rb.m_pos;
	toPlayer.y = 0.0f;
	const float distance = toPlayer.Magnitude();

	//上から順に条件を見て、当てはまったらその攻撃
	//何回か攻撃したら全体攻撃
	if (m_attackCountSinceLarge >= kLargeAttackInterval &&
		distance <= GetAttackNode(kLargeRangedAttackIndex).attackRange)
	{
		return kLargeRangedAttackIndex;
	}
	//背後にいたら薙ぎ払い
	Vector3 front = m_targetVec;
	front.y = 0.0f;
	if (distance > 0.0f && front.Magnitude() > 0.0f &&
		distance <= GetAttackNode(kSweepAttackIndex).attackRange &&
		front.Normalize().Dot(toPlayer.Normalize()) < kBackDot)
	{
		return kSweepAttackIndex;
	}
	//遠かったら突進
	if (distance >= kRushMinDistance && distance <= kRushMaxDistance)
	{
		return kRushAttackIndex;
	}

	//どれにも当てはまらなければ、横斬りかコンボを半分ずつ(距離に入っているものだけ)
	std::vector<int> candidates;
	for (const int index : { kNormalAttackIndex, kComboAttackIndex })
	{
		if (distance <= GetAttackNode(index).attackRange)
		{
			candidates.push_back(index);
		}
	}
	if (candidates.empty())return -1;

	//ランダムで1つ選ぶ//GetRand(n)は0〜nを返す
	const int pick = GetRand(static_cast<int>(candidates.size()) - 1);
	return candidates[pick];
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextAfterAttackEnd()
{
	//連撃が終わったら攻撃の回数を数える//全体攻撃なら0に戻す
	//何回か攻撃したら大技を繰り出すよう
	if (GetAttackNode(m_currentAttackIndex).nextAttack < 0)
	{
		if (m_currentAttackIndex == kLargeRangedAttackIndex)
		{
			m_attackCountSinceLarge = 0;
		}
		else
		{
			++m_attackCountSinceLarge;
		}
	}

	//連撃//今の攻撃にnextAttackがあれば、予備動作なしで次の攻撃へ
	const int next = GetAttackNode(m_currentAttackIndex).nextAttack;
	if (next >= 0)
	{
		assert(next < static_cast<int>(m_attackList.size()) && "nextAttackが攻撃CSVの行数を超えています");
		m_currentAttackIndex = next;
		return std::make_shared<EnemyAttack>(GetWeakPtr(), true);
	}
	//アイドルに戻す
	return std::make_shared<EnemyIdle>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextAfterIdle()
{
	//ボスはすぐ攻撃//出せる攻撃が無ければIdleのまま
	//あとでSelectAttackIndexを変えないといけない
	const int attackIndex = SelectAttackIndex();
	auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock();
	if (attackIndex >= 0 && tokenMgr && tokenMgr->Request(*this))
	{
		m_currentAttackIndex = attackIndex;
		return std::make_shared<EnemyAttackPre>(GetWeakPtr());
	}
	return std::make_shared<EnemyIdle>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextOnArrived()
{
	return EnemyBase::NextOnArrived();
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextAfterCaution()
{
	return EnemyBase::NextAfterCaution();
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextAfterAttack()
{
	return EnemyBase::NextAfterAttack();
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextAfterParry()
{
	return EnemyBase::NextAfterParry();
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextOnPlayerFound()
{
	return EnemyBase::NextOnPlayerFound();
}

std::shared_ptr<EnemyStateBase> BossEnemyKingmech::NextAfterRecover()
{
	return EnemyBase::NextAfterRecover();
}

void BossEnemyKingmech::OnDamage(Collider& other, AttackData& data)
{
	auto player = m_player.lock();
	if (!player)return;

	//処刑済み(確殺・暗殺中)なら被弾しない(演出中のStateが上書きされないようにする)
	if (m_isExecuted)return;
	//死亡済み・死亡吹っ飛び中も被弾しない//ガードより先に見ないと、死体がガードのStateに入ってしまう
	if (m_isLifeZero || m_isDead || m_isDieOut)return;

	//データの保存
	m_attackData = data;

	//攻撃中だったら//スタン値がたまるなどしない限りボスの攻撃は止まらない
	const bool isAttacking = IsAttackFlowState(m_currentState);
	if (isAttacking)
	{
		//ダメージとエフェクトだけ出す//ガード・ヒットステートには行かない
		m_hp -= static_cast<int>(data.attackPower);
		m_isPlayerFound = true;
		PlayBloodSplash(other);

		//倒れたらその場で死ぬ
		if (m_hp <= 0)
		{
			m_hp = 0;
			ChangeState(std::make_shared<EnemyDie>(GetWeakPtr()));
		}
		return;
	}

	//OnCollision中はIsFloor()がfalseのままなので前フレームの接地状態を見る
	const bool isFloor = WasFloor();

	//ガード
	//攻撃が弱攻撃ならガード//かつ地上だったら
	bool isForceHit = false;
	if (data.attackType == AttackType::lightAttack && isFloor)
	{
		//確定ヒット中は抽選しない
		if (m_forceHitCount > 0)
		{
			--m_forceHitCount;
			isForceHit = true;
		}
		//ここから確率
		else if (Utility::IsHitRate(static_cast<int>(m_status.guardRate)))
		{
			//ガードブレイク中・攻撃の後隙中はガードに移行しない
			if (!std::dynamic_pointer_cast<EnemyGuardBreak>(m_currentState) &&
				!std::dynamic_pointer_cast<EnemyAttackEnd>(m_currentState))
			{
				//ガード中ではないなら
				if (!std::dynamic_pointer_cast<EnemyGuard>(m_currentState))
				{
					ChangeState(std::make_shared<EnemyGuard>(GetWeakPtr()));
					return;
				}
			}
		}
	}

	//ガード中なら
	if (std::dynamic_pointer_cast<EnemyGuard>(m_currentState))
	{
		//もし、スキル攻撃や必殺技が当たったらそのままダメージ処理へ進む
		if (data.attackType == AttackType::SkillAttack || data.attackType == AttackType::UltAttack)
		{
			//そのままダメージ処理
		}
		//それらと違うなら
		else
		{
			auto currentState = std::dynamic_pointer_cast<EnemyGuard>(m_currentState);
			//ガードしているときにHit
			currentState->OnGuardHit();
			return;
		}
	}

	//被弾したら、ここから数段は確定ヒット
	if (!isForceHit && m_forceHitCount <= 0)
	{
		m_forceHitCount = m_status.forceHitCount;
	}
	m_forceHitTimer = 0.0f;

	//Playerの攻撃データをもとに被ダメ処理をする
	m_hp -= static_cast<int>(data.attackPower);

	//被ダメをしたらplayerを見つけた判定にする
	m_isPlayerFound = true;

	//血しぶきを再生する
	PlayBloodSplash(other);

	//Enemy->Playerのベクトルに吹き飛ばす力を加える//プレイヤーの正面に行くようにknockBackする
	//吸着させる
	Vector3 front = player->GetTargetVec();
	Vector3 pos = player->GetRigidBody().GetPos();
	Vector3 TargetPos = pos + front * kEnemyDistance;
	Vector3 toTarget = (TargetPos - m_rb.m_pos).Normalize() * kToTargetPower;

	Vector3 pushBackVec = (m_rb.m_pos - other.GetRigidBody().GetPos()).Normalize() *
		data.knockBackPower.x;
	pushBackVec += toTarget;

	//ヒット情報の作成
	HitInfo hitinfo = {
		.knockBackVel = Vector3(pushBackVec.x,data.knockBackPower.y,pushBackVec.z),//Y軸の上下降はここで加える
		.knockBackPowerXZ = data.knockBackPower.x,
		.duration = data.knockBackFrame,
		.isKirimomi = data.isKirimomi,
	};

	//体力が0になったときの処理
	if (m_hp <= 0)
	{
		const bool isDropHit = data.knockBackPower.y < 0.0f || hitinfo.isKirimomi;
		if (isDropHit)
		{
			//叩きつけ・キリモミ吹っ飛びは、吹き飛んでから着地したときに死ぬ(EnemyHitDropがEnemyDieに切り替える)
			m_hp = 0;
			m_isDieOut = true;
			if (m_hitCol) m_hitCol->SetIsActive(false);
		}
		else if (isFloor && data.knockBackPower.y <= 0.0f)
		{
			//地上で普通の攻撃を受けたらその場で死ぬ
			m_hp = 0;
			ChangeState(std::make_shared<EnemyDie>(GetWeakPtr()));
			return;
		}
		else
		{
			//空中・打ち上げ中は死なない(空中コンボを続けられるように、体力を1残す)
			m_hp = 1;
		}
	}

	//ガードブレイク中ならヒットステートじゃなくてガードステートのまま
	//ガードブレイク攻撃の時はしっかり移行させる
	bool isGuardBreakAttack = data.attackType == AttackType::GuardBreakAttack;
	if (std::dynamic_pointer_cast<EnemyGuardBreak>(m_currentState) && !isGuardBreakAttack)
	{
		auto state = std::dynamic_pointer_cast<EnemyGuardBreak>(m_currentState);

		//遷移がダメならリターンする
		if (!state->CanChangeHit())return;
	}

	//ボスは空中とかに行かないし、攻撃中はヒットリアクションを取らない
	//isKirimomiのような大きい攻撃は大きくのけぞる
	if (hitinfo.isKirimomi)
	{
		ChangeState(std::make_shared<EnemyHitLargeHit>(GetWeakPtr(), hitinfo));
	}
	else
	{
		ChangeState(std::make_shared<EnemyHitGround>(GetWeakPtr(), hitinfo));
	}
}

void BossEnemyKingmech::OnAssasined()
{
	EnemyBase::OnAssasined();
}

void BossEnemyKingmech::OnHeadBreak()
{
	m_isPartBroken = true;
}

void BossEnemyKingmech::OnPartBreak()
{
	m_isPartBroken = true;
}
