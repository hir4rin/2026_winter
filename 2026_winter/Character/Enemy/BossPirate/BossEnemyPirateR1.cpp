#include "BossEnemyPirateR1.h"
#include "DataLoader/DataManager.h"
#include "Main/System.h"
#include "Math/Matrix4x4.h"
#include "Collision/SphereShape.h"
#include "Collision/CapsuleShape.h"
#include "Character/Enemy/EnemyPart.h"
#include "Collision/HitCol.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/State/Locomotion/EnemyIdle.h"
#include "Character/Enemy/State/Locomotion/EnemyPatrol.h"
#include "Main/Input.h"
#include "Render/ModelShaderManager.h"
#include "Character/Enemy/BossPirate/Part/BossPirateWeapon.h"
#include "Battle/BattleManager.h"
#include "Battle/AttackTokenManager.h"
#include "Character/Enemy/BossPirate/Part/BossArm.h"
#include "Character/Enemy/BossPirate/State/BossGuard.h"
#include "Character/Enemy/BossPirate/State/BossGuardBreak.h"
#include "Main/imguiApp.h"
#include "imgui.h"



namespace
{
	const std::string kStatusType = "BossEnemyPirateR1";//EnemyStatus.csvのtype

	constexpr float kBreakPartScale = 0.001f;//部位破壊したボーンの大きさ//0だと法線が壊れるので少しだけ残す

	constexpr float kPartColliderRadius = 15.0f;//切り離したパーツの当たり判定の半径

	constexpr float kPartJumpUpSpeed = 15.0f;//切り離した瞬間の上向きの速度
	constexpr float kPartJumpSideSpeed = 6.0f;//切り離した瞬間の横方向(敵の右側)の速度

	constexpr float kModelScale = 2.5f;//モデルの大きさ


	constexpr float kToTargetPower = 3.0f;//プレイヤーの正面に行くようにknockBackする力


	constexpr float kEnemyDistance = 50.0f;

	constexpr int kNormalAttackIndex = 0;//横斬り
	constexpr int kLargeRangedAttackIndex = 1;
	constexpr int kComboAttackIndex = 3;//コンボの1撃目
	constexpr int kSweepAttackIndex = 2;//周囲薙ぎ払い
	constexpr int kRushAttackIndex = 6;//とびかかり(突進)

	constexpr int kLargeAttackInterval = 10;//この回数攻撃したら全体攻撃
	constexpr float kBackDot = -0.5f;//正面との内積がこれより小さければ背後(120度より後ろ)
	constexpr float kRushMinDistance = 400.0f;//この距離より遠ければ突進
	constexpr float kRushMaxDistance = 1000.0f;//突進を出す最大の距離

}


BossEnemyPirateR1::BossEnemyPirateR1(std::weak_ptr<Player> player, Vector3 startPos):EnemyBase(player)
{

	//ステータス
	InitializeStatus(DataManager::GetInstance().GetEnemyStatusRawData(), kStatusType);
	//攻撃のデータ
	InitializeAttackList(DataManager::GetInstance().GetBossEnemyPirateR1AttackRawData());

	m_rb.m_pos = startPos;
	m_hp = m_status.hp;

	m_modelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::BossModel));
	m_waistFrame = MV1SearchFrame(m_modelHandle, "pelvis");//腰のボーン
	m_modelScale = kModelScale;
	//部位破壊のパーツのモデル//読み込みはSystemで済ませているので複製する
	m_leftArmModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyPartLeftArmModel));
	m_headModelHandle = MV1DuplicateModel(System::GetInstance().GetHandle(AsyncData::EnemyPartHeadModel));

	m_leftArmPart = std::make_shared<EnemyPart>(m_leftArmModelHandle);
	m_headPart = std::make_shared<EnemyPart>(m_headModelHandle);

	//モデルの初期位置を設定する
	Matrix4x4 rotY = Matrix4x4::MakeRotationY(0);
	MATRIX transmat = MGetTranslate(m_rb.m_pos.ToDxLibVector());
	Matrix4x4 trans = Matrix4x4::FromDxLibMatrix(transmat);
	Matrix4x4 scale = Matrix4x4::FromDxLibMatrix(MGetScale(VGet(kModelScale, kModelScale, kModelScale)));
	Matrix4x4 mtx = trans * rotY * scale;
	MV1SetMatrix(m_modelHandle, Matrix4x4::ToDxLibMatrix(mtx));

	//アニメーションの名前のマップの初期化
	const auto& animData = DataManager::GetInstance().GetBossEnemyPirateR1AnimData();

	m_animNames = animData.animNames;
	m_animSpeeds = animData.animSpeeds;//アニメCSVの再生速度

	
	
}

BossEnemyPirateR1::~BossEnemyPirateR1()
{
	if (m_currentState)
	{
		m_currentState->Exit();//状態を抜ける
	}
	MV1DeleteModel(m_modelHandle);
	MV1DeleteModel(m_leftArmModelHandle);
	MV1DeleteModel(m_headModelHandle);
}

void BossEnemyPirateR1::Init()
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

	m_weapon = std::make_shared<BossPirateWeapon>(m_modelHandle);//剣の生成
}

void BossEnemyPirateR1::Update()
{
	//押し戻しの処理が続かないように消す//縦の速度(重力)は空中のステート(HitAir,HitDrop,AirFall)が自分で作る
	m_rb.m_vel = Vector3(0, 0, 0);
	auto& input = Input::GetInstance();

	//体力0なら更新しない
	//なんかバグったので、一旦しない
	if (m_isLifeZero)
	{
		////アニメーションの更新
		//m_anim.Update();
		//return;
	}

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

	//デバッグ用//Qを押した瞬間だけBossEnemyPirateR1Attack.csvを読み込み直して攻撃データを更新する
	bool isQPressed = CheckHitKey(KEY_INPUT_Q) != 0;
	if (isQPressed && !m_wasQPressed)
	{
		DataManager::GetInstance().ReloadBossEnemyPirateR1AttackRawData();
		InitializeAttackList(DataManager::GetInstance().GetBossEnemyPirateR1AttackRawData());
		DataManager::GetInstance().ReloadEnemyStatusRawData();
		InitializeStatus(DataManager::GetInstance().GetEnemyStatusRawData(), kStatusType);
	}
	m_wasQPressed = isQPressed;

	//デバッグ用//Iを押した瞬間だけBossEnemyPirateR1Anim.csvを読み込み直す//次にアニメが切り替わったときから効く
	bool isIPressed = CheckHitKey(KEY_INPUT_I) != 0;
	if (isIPressed && !m_wasIPressed)
	{
		DataManager::GetInstance().ReloadBossEnemyPirateR1AnimData();
		const auto& animData = DataManager::GetInstance().GetBossEnemyPirateR1AnimData();
		m_animNames = animData.animNames;
		m_animSpeeds = animData.animSpeeds;
	}
	m_wasIPressed = isIPressed;

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

	//もし被弾中で攻撃のクールタイムが上がったら攻撃に移行
	if (std::dynamic_pointer_cast<EnemyHitGround>(m_currentState))
	{
		if (m_attackCoolTime <= 0.0f)
		{
			//ランダムでChaseかCautionに遷移する
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
	//剣を右手に合わせる//アニメーションの更新の後
	m_weapon->Update();
}

void BossEnemyPirateR1::Draw()
{
	ModelShaderManager::GetInstance().DrawModel(m_modelHandle);//血殺中は黒く描く
	m_weapon->Draw();//剣
	//部位破壊したパーツの描画
	if (m_isBreakLeftArm) m_leftArmPart->Draw();
	if (m_isBreakHead) m_headPart->Draw();
}

void BossEnemyPirateR1::DebugDraw()
{
	EnemyBase::DebugDraw();

	//背中の腕の付け根・ずれの調整(F2で表示/非表示)
	if (!imguiApp::GetInstance().IsDebugUIVisible())return;
	auto leftArm = m_leftBackArm.lock();
	auto rightArm = m_rightBackArm.lock();
	if (!leftArm || !rightArm)return;

	ImGui::SetNextWindowSize(ImVec2(380, 420), ImGuiCond_FirstUseEver);
	ImGui::Begin("Boss Arm Editor");
	if (ImGui::Button("Mirror Left -> Right"))
	{
		rightArm->SetMirrorOffset(*leftArm);
	}
	leftArm->DrawEditor();
	rightArm->DrawEditor();
	ImGui::End();
}

std::vector<std::shared_ptr<EnemyBase>> BossEnemyPirateR1::CreateChildEnemies()
{
	auto leftArm = std::make_shared<BossArm>(m_player, GetWeakPtr(), m_modelHandle, BossArm::Side::Left);
	auto rightArm = std::make_shared<BossArm>(m_player, GetWeakPtr(), m_modelHandle, BossArm::Side::Right);
	leftArm->Init();
	rightArm->Init();
	m_leftBackArm = leftArm;
	m_rightBackArm = rightArm;
	return { leftArm, rightArm };
}

void BossEnemyPirateR1::OnCollision(Collider& other)
{
}

int BossEnemyPirateR1::SelectAttackIndex()
{
	//クールタイム中は攻撃しない
	if (m_attackCoolTime > 0.0f)return -1;

	//return 6;

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
	//横きり、コンボ1の攻撃のどちらか
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

std::shared_ptr<EnemyStateBase> BossEnemyPirateR1::NextAfterAttackEnd()
{
	//連撃が終わったら攻撃の回数を数える//全体攻撃なら0に戻す
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
	return EnemyBase::NextAfterAttackEnd();
}

void BossEnemyPirateR1::OnDamage(Collider& other, AttackData& data)
{
	//一旦EnemyBaseからコピーしてきた


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
	//一旦100移行
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
			if (!std::dynamic_pointer_cast<BossGuardBreak>(m_currentState) &&
				!std::dynamic_pointer_cast<EnemyAttackEnd>(m_currentState))
			{
				//ガード中ではないなら
				if (!std::dynamic_pointer_cast<EnemyGuard>(m_currentState))
				{
					ChangeState(std::make_shared<BossGuard>(GetWeakPtr()));//背中の腕もガードする
					return;
				}
			}
		}
	}

	//ガード中なら
	if (std::dynamic_pointer_cast<BossGuard>(m_currentState))
	{
		//もし、スキル攻撃や必殺技が当たったらそのままダメージ処理へ進む
		if (data.attackType == AttackType::SkillAttack || data.attackType == AttackType::UltAttack)
		{
			//そのままダメージ処理
		}
		//それらと違うなら
		else
		{
			auto currentState = std::dynamic_pointer_cast<BossGuard>(m_currentState);
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


	//部位破壊率の確率で部位破壊する//部位破壊 * 被部位破壊率
	if (!m_isPartBroken && Utility::IsHitRate(static_cast<int>(data.brokenRate * m_status.partBreakRate)))
	{
		//OnPartBreak();
	}




	////ダメージがあるなら、ヒットエフェクトを再生する//必殺技の時は、ヒットエフェクトをスローのものにする
	//if (static_cast<int>(data.attackPower) > 0)
	//{
	//	//必殺技
	//	if (data.attackPower >= kUltDamagePower)
	//	{
	//		//m_hitEfPlayingHandle = PlayEffekseer3DEffectSlow(m_hitEfHandle, 0.5f);
	//	}
	//	//その他
	//	else
	//	{
	//		m_hitEfPlayingHandle = PlayEffekseer3DEffect(m_hitEfHandle);
	//		SetPosPlayingEffekseer3DEffect(m_hitEfPlayingHandle, m_pos.x, m_pos.y + kEnemyEfOffset, m_pos.z);
	//	}
	//}

	//if (m_hp <= 0)
	//{
	//	//空中じゃ死なない
	//	if (!IsFloor())
	//	{
	//		m_hp = 1;
	//	}
	//	else
	//	{
	//		m_hp = 0;
	//		m_isLifeZero = true;
	//		//当たり判定を解除する
	//		Terminate();

	//		//キリモミ吹っ飛びの時は、途中で死ぬ
	//		if (m_attackData.isKirimomi)
	//		{
	//			m_isDieOut = true;
	//		}
	//		//死亡アニメーションに移行
	//		else
	//		{
	//			ChangeState(EnemyState::Dead);
	//			return;
	//		}
	//	}
	//}

	//Enemy->Playerのベクトルに吹き飛ばす力を加える//プレイヤーの正面に行くようにknockBackする//いずれkirimomi吹っ飛びの時の処理と分ける
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
			//このまま下のHitDropへ進む
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

	//Stateの切り替え//敵を吹き飛ばす攻撃かどうかで切り替える
	//ガードブレイク中ならヒットステートじゃなくてガードステートのまま
	//ガードブレイク攻撃の時はしっかり移行させる
	bool isGuardBreakAttack = data.attackType == AttackType::GuardBreakAttack;
	if (std::dynamic_pointer_cast<BossGuardBreak>(m_currentState))
	{
		auto state = std::dynamic_pointer_cast<BossGuardBreak>(m_currentState);
		//遷移がダメならリターンする
		if (!state->CanChangeHit())return;
	}
	//ボスは空中とかに行かないし、攻撃中はヒットリアクションを取らない。部位破壊もラウンド0はしない
	//よろめいた感じとパターンは出す

	//isKirimomiのような大きい攻撃は大きくのけぞる
	if (hitinfo.isKirimomi)
	{
		ChangeState(std::make_shared<EnemyHitLargeHit>(GetWeakPtr(), hitinfo));
		return;
	}
	else
	{
		//既にHitStateの時はやらない
		//if (!std::dynamic_pointer_cast<EnemyHitGround>(m_currentState))
		//{
			ChangeState(std::make_shared<EnemyHitGround>(GetWeakPtr(), hitinfo));
		//}
		return;
	}

	if (data.knockBackPower.y > 0.0f && !hitinfo.isKirimomi)
	{
		//上昇の時の攻撃
		ChangeState(std::make_shared<EnemyHitAir>(GetWeakPtr(), hitinfo));
		return;
	}
	else if (data.knockBackPower.y < 0.0f || hitinfo.isKirimomi)
	{
		//下降時の攻撃、もしくは吹っ飛び時の攻撃
		ChangeState(std::make_shared<EnemyHitDrop>(GetWeakPtr(), hitinfo));
		return;
	}
	else
	{
		//敵が空中にいる状態になるのを防ぐ
		//吹き飛ばし攻撃中は遷移しない
		if (std::dynamic_pointer_cast<EnemyHitAir>(m_currentState) ||
			std::dynamic_pointer_cast<EnemyHitDrop>(m_currentState))
		{
			return;
		}
		//普通の攻撃
		ChangeState(std::make_shared<EnemyHitGround>(GetWeakPtr(), hitinfo));
	}
}

void BossEnemyPirateR1::OnAssasined()
{
	EnemyBase::OnAssasined();
}

void BossEnemyPirateR1::OnHeadBreak()
{
	m_isPartBroken = true;
	SetUpBreakHead();
	//SetUpBreakLeftArm();
}

void BossEnemyPirateR1::OnPartBreak()
{
	if (m_isBreakLeftArm)return;
	m_isPartBroken = true;
	SetUpBreakLeftArm();
}

void BossEnemyPirateR1::SetOpacity(float rate)
{
	EnemyBase::SetOpacity(rate);
	MV1SetOpacityRate(m_leftArmModelHandle, rate);
	MV1SetOpacityRate(m_headModelHandle, rate);
	if (m_weapon) m_weapon->SetOpacity(rate);
	//背中の腕も一緒に消える
	if (auto leftArm = m_leftBackArm.lock()) leftArm->SetOpacity(rate);
	if (auto rightArm = m_rightBackArm.lock()) rightArm->SetOpacity(rate);
}

void BossEnemyPirateR1::SetUpBreakLeftArm()
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

void BossEnemyPirateR1::SetUpBreakHead()
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
