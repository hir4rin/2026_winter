#include "EnemyBase.h"
#include "Character/Enemy/State/EnemyStateBase.h"
#include "Character/Player/Player.h"
#include "Collision/HitCol.h"
#include "Main/Game.h"
#include "Main/System.h"
#include "../../DataLoader/DataManager.h"
#include "Battle/BattleManager.h"
#include "Render/EffectManager.h"
#include "Battle/AttackTokenManager.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <format>

namespace
{
	//PatrolRouteX.csvの列番号
	enum PatrolRouteColumn : int
	{
		PosX = 0,
		PosY = 1,
		PosZ = 2,
		WaitTime = 3,
		Index = 4,
		Size,//列数
	};

	constexpr float kCautionBackVecRate = 0.2f;//半径を維持するためのベクトルの倍率


	constexpr float kToTargetPower = 3.0f;//プレイヤーの正面に行くようにknockBackする力


	constexpr float kEnemyDistance = 50.0f;

	//血しぶきエフェクト
	constexpr int kBloodSplashPatternNum = 3;//パターン数(AsyncData::BloodSplashEffectA〜C)
	constexpr float kBloodSplashHeight = 100.0f;//足元からの高さ(やられ判定の高さに合わせる)
	constexpr float kBloodSplashScale = 20.0f;//エフェクトは1/20で作っているので20倍して元の大きさに戻す
	constexpr float kBloodSplashUltSpeed = 0.45f;//必殺技中の血しぶきの再生速度(タイムスケールに関係なくこの速さになる)

	//デバッグ表示
	constexpr float kDebugTextHeight = 220.0f;//文字を出す高さ(足元から)
	constexpr float kDebugLineHeight = 30.0f;//向き・踏み込み先の線を引く高さ(足元から)//地面に埋もれないように少し上げる
	constexpr float kDebugForwardLength = 150.0f;//向きの線の長さ
	constexpr float kDebugForwardTipRadius = 8.0f;//向きの線の先の球の半径
	constexpr int kDebugSphereDivNum = 8;//球の分割数
	constexpr int kDebugLineSpacing = 2;//文字の行間
	constexpr float kDebugTokenRingRadius = 80.0f;//攻撃権の輪の半径
	constexpr int kDebugTokenRingDivNum = 24;//攻撃権の輪の分割数
}

EnemyBase::EnemyBase(std::weak_ptr<Player> player)
	: m_player(player)
{
}

EnemyBase::~EnemyBase()
{
}

void EnemyBase::SetPatrolRoute(int routeId)
{
	//CSVファイルを読み込む
	const auto& rawData = DataManager::GetInstance().GetPatrolRouteRawData();

	m_patrolPoints.clear();//設定し直しのときに前のデータが残らないように消す
	m_currentPatrolIndex = -1;//最初のポイントから巡回し直す

	if (routeId < 0 || routeId >= static_cast<int>(rawData.size()))
	{
		assert(false && "存在しない巡回ルート番号です");
		return;
	}

	for (const auto& tokens : rawData[routeId])
	{
		//列数チェック//tokensは1行分のデータ
		if (tokens.size() < PatrolRouteColumn::Size)
		{
			assert(false && "PatrolRoute.csvの列数が不足しています");
			continue;
		}
		PatrolPoint point;
		point.pos = Vector3(std::stof(tokens[PatrolRouteColumn::PosX]),
			std::stof(tokens[PatrolRouteColumn::PosY]),
			std::stof(tokens[PatrolRouteColumn::PosZ]));
		point.waitTime = std::stof(tokens[PatrolRouteColumn::WaitTime]);
		point.index = std::stoi(tokens[PatrolRouteColumn::Index]);
		m_patrolPoints.push_back(point);
	}

	//CSVの行の順番ではなく、indexの順番で巡回する
	std::sort(m_patrolPoints.begin(), m_patrolPoints.end(),
		[](const PatrolPoint& a, const PatrolPoint& b) { return a.index < b.index; });
}

void EnemyBase::SetFacing(float rotYDeg)
{
	float rad = rotYDeg * DX_PI_F / 180.0f;
	//向きたい方向//Unityと同じで、0度なら+Z、90度なら+Xを向く
	m_targetVec = Vector3(sinf(rad), 0.0f, cosf(rad));
	//UpdateAngleAndPosがm_targetVecに少しずつ近づけるので、最初から向いた状態にしておく
	//(モデルが180度ずれているので、UpdateAngleAndPosと同じくDX_PI_Fずらす)
	m_rotAngleY = rad - DX_PI_F;
}

void EnemyBase::SetPatrolPoints(const std::vector<EnemySpawnRoutePoint>& route)
{
	m_patrolPoints.clear();//設定し直しのときに前のデータが残らないように消す
	m_currentPatrolIndex = -1;//最初のポイントから巡回し直す

	for (int i = 0; i < static_cast<int>(route.size()); ++i)
	{
		PatrolPoint point;
		point.pos = route[i].pos;
		point.waitTime = route[i].waitTime;
		point.index = i;
		m_patrolPoints.push_back(point);
	}
}

void EnemyBase::SetWatchPoint()
{
	//今の位置で、ずっと待機する(waitTimeが負なら次へ出発しない)
	SetPatrolPoints({ { m_rb.m_pos, -1.0f } });
}

void EnemyBase::OnAlerted()
{
	if (m_isPlayerFound)return;//もう気づいている
	if (m_isLifeZero || m_isExecuted)return;
	//巡回・見張り中のときだけ追跡を始める(戦闘中や被弾中のStateは上書きしない)
	if (!std::dynamic_pointer_cast<EnemyPatrol>(m_currentState))return;

	m_isPlayerFound = true;
	ChangeState(NextOnPlayerFound());
}

void EnemyBase::SetDebugIdle(bool isDebugIdle)
{
	//デバッグウィンドウでAIを止めているときも同じ扱い
	m_isDebugIdleGlobal = isDebugIdle;
	m_isDebugIdle = isDebugIdle || m_isDebugStopAI;
	if (!m_isDebugIdle)return;
	if (m_isLifeZero || m_isExecuted)return;

	//巡回・追跡・警戒・後退中ならIdleにする//被弾中のStateはそのまま(終わったら自分でIdleに戻る)
	if (std::dynamic_pointer_cast<EnemyPatrol>(m_currentState) ||
		std::dynamic_pointer_cast<EnemyChase>(m_currentState) ||
		std::dynamic_pointer_cast<EnemyCaution>(m_currentState) ||
		std::dynamic_pointer_cast<EnemyBack>(m_currentState))
	{
		ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));
		return;
	}
	//攻撃中もIdleにする//デバッグウィンドウで止めているだけなら、選んだ攻撃を出せるように残す
	if (isDebugIdle && IsAttackFlowState(m_currentState))
	{
		ChangeState(std::make_shared<EnemyIdle>(GetWeakPtr()));
	}
}

unsigned int EnemyBase::GetDebugCategoryColor(EnemyStateBase::DebugCategory category)
{
	switch (category)
	{
	case EnemyStateBase::DebugCategory::AttackPre: return GetColor(255, 160, 0);//オレンジ
	case EnemyStateBase::DebugCategory::Attack:    return GetColor(255, 60, 60);//赤
	case EnemyStateBase::DebugCategory::Recovery:  return GetColor(80, 255, 80);//緑
	case EnemyStateBase::DebugCategory::Hit:       return GetColor(255, 255, 0);//黄
	case EnemyStateBase::DebugCategory::Guard:     return GetColor(0, 220, 255);//水色
	case EnemyStateBase::DebugCategory::Dead:      return GetColor(128, 128, 128);//灰
	default:                                       return GetColor(255, 255, 255);//白
	}
}

void EnemyBase::DrawDebugTextCenter(int x, int y, const std::string& text, unsigned int color)
{
	const int width = GetDrawStringWidth(text.c_str(), static_cast<int>(text.size()));
	DrawString(x - width / 2 + 1, y + 1, text.c_str(), GetColor(0, 0, 0));
	DrawString(x - width / 2, y, text.c_str(), color);
}

void EnemyBase::DebugDraw()
{
	if (!m_currentState)return;

	//ステートごとの3D描画(巡回経路・視界など)
	m_currentState->DebugDraw();

	const auto category = m_currentState->GetDebugCategory();
	const unsigned int color = GetDebugCategoryColor(category);
	const Vector3 lineStart = m_rb.m_pos + Vector3(0, kDebugLineHeight, 0);

	if (category != EnemyStateBase::DebugCategory::Dead)
	{
		//向いている方向(m_targetVec)//予備動作の途中で向きが固定されるかの確認用
		Vector3 forward = m_targetVec;
		forward.y = 0.0f;
		if (forward.Magnitude() > 0.0f)
		{
			const Vector3 tip = lineStart + forward.Normalize() * kDebugForwardLength;
			DrawLine3D(lineStart.ToDxLibVector(), tip.ToDxLibVector(), color);
			DrawSphere3D(tip.ToDxLibVector(), kDebugForwardTipRadius, kDebugSphereDivNum, color, color, true);
		}
	}

	//予備動作・攻撃中は踏み込み先(m_targetPos)への線
	if (category == EnemyStateBase::DebugCategory::AttackPre ||
		category == EnemyStateBase::DebugCategory::Attack)
	{
		const Vector3 target(m_targetPos.x, lineStart.y, m_targetPos.z);
		DrawLine3D(lineStart.ToDxLibVector(), target.ToDxLibVector(), color);
	}

	//攻撃権を持っていたら足元に輪を出す
	bool hasToken = false;
	if (auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock())
	{
		hasToken = tokenMgr->HasToken(*this);
	}
	const unsigned int tokenColor = GetColor(255, 0, 255);
	if (hasToken)
	{
		//分割して円を下向きに表示
		for (int i = 0; i < kDebugTokenRingDivNum; i++)
		{
			const float angle0 = DX_TWO_PI_F * i / kDebugTokenRingDivNum;
			const float angle1 = DX_TWO_PI_F * (i + 1) / kDebugTokenRingDivNum;
			const Vector3 p0 = lineStart + Vector3(cosf(angle0), 0, sinf(angle0)) * kDebugTokenRingRadius;
			const Vector3 p1 = lineStart + Vector3(cosf(angle1), 0, sinf(angle1)) * kDebugTokenRingRadius;
			DrawLine3D(p0.ToDxLibVector(), p1.ToDxLibVector(), tokenColor);
		}
	}

	//頭の上に文字を出す//カメラの後ろにいる(zが0〜1の外)ときは出さない
	const Vector3 textPos = m_rb.m_pos + Vector3(0, kDebugTextHeight, 0);
	const VECTOR screenPos = ConvWorldPosToScreenPos(textPos.ToDxLibVector());
	if (screenPos.z <= 0.0f || screenPos.z >= 1.0f)return;

	std::string stateText = m_currentState->GetName();
	const std::string info = m_currentState->GetDebugInfo();
	if (!info.empty()) stateText += " " + info;
	const std::string statusText = std::format("HP {} CT {:.0f}", m_hp, (std::max)(m_attackCoolTime, 0.0f));

	const int x = static_cast<int>(screenPos.x);
	const int y = static_cast<int>(screenPos.y);
	const int lineHeight = GetFontSize() + kDebugLineSpacing;
	int topY = y - lineHeight * 2;
	if (hasToken)
	{
		DrawDebugTextCenter(x, topY, "TOKEN", tokenColor);
		topY -= lineHeight;
	}
	//グループ名(あれば)
	if (!m_groupId.empty())DrawDebugTextCenter(x, topY, "Group:" + m_groupId, GetColor(0, 255, 0));
	DrawDebugTextCenter(x, y - lineHeight, stateText, color);
	DrawDebugTextCenter(x, y, statusText, GetColor(255, 255, 255));
}

void EnemyBase::OnCollision(Collider& other)
{
}

void EnemyBase::OnDamage(Collider& other, AttackData& data)
{
	auto player = m_player.lock();
	if (!player)return;

	//処刑済み(確殺・暗殺中)なら被弾しない(演出中のStateが上書きされないようにする)
	if (m_isExecuted)return;
	//死亡済み・死亡吹っ飛び中も被弾しない//ガードより先に見ないと、死体がガードのStateに入ってしまう
	if (m_isLifeZero || m_isDead || m_isDieOut)return;

	//データの保存
	m_attackData = data;

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
	if(std::dynamic_pointer_cast<EnemyGuard>(m_currentState))
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


	//部位破壊率の確率で部位破壊する//部位破壊 * 被部位破壊率
	if (!m_isPartBroken && Utility::IsHitRate(static_cast<int>(data.brokenRate * m_status.partBreakRate)))
	{
		OnPartBreak();
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
	if (std::dynamic_pointer_cast<EnemyGuardBreak>(m_currentState) && !isGuardBreakAttack)
	{
		auto state = std::dynamic_pointer_cast<EnemyGuardBreak>(m_currentState);

		//遷移がダメならリターンする
		if(!state->CanChangeHit())return;
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

void EnemyBase::OnAssasined()
{
	//処刑済みにする(もう一度暗殺・確殺の対象にならないようにする)
	m_isExecuted = true;
	//StateをAssasinに変える
	ChangeState(std::make_shared<EnemyAssasined>(GetWeakPtr()));
	return;
}

void EnemyBase::OnAirAssasined()
{
	//処刑済みにする(もう一度暗殺・確殺の対象にならないようにする)
	m_isExecuted = true;
	//StateをAirAssasinに変える
	ChangeState(std::make_shared<EnemyAirAssasined>(GetWeakPtr()));
	return;
}

void EnemyBase::OnPartBrokenKilled(PartBrokenPattern pattern)
{
	//処刑済みにする(もう一度暗殺・確殺の対象にならないようにする)
	m_isExecuted = true;
	//StateをPartBrokenKilledに変える(パターンはコンストラクタで渡す)
	ChangeState(std::make_shared<EnemyPartBrokenKilled>(GetWeakPtr(), pattern));
	return;
}

void EnemyBase::UpdateLookAtPlayer(int headFrame)
{
	//頭を破壊していたらこれを更新しない//今は部位破壊をしていたらにする
	//if (m_isPartBroken) return;

	auto player = m_player.lock();
	if (!player)return;

	//Chase,Caution,Idleのときだけプレイヤーの方を向く//回す処理はCharacterBaseと共通
	UpdateHeadLook(headFrame, m_isLookAtPlayer, player->GetRigidBody().GetPos());
}

void EnemyBase::StartVanish()
{
	//すでに消えている途中なら何もしない
	if (m_isVanishing)return;
	ChangeState(std::make_shared<EnemyVanish>(GetWeakPtr()));
}

void EnemyBase::SetOpacity(float rate)
{
	MV1SetOpacityRate(m_modelHandle, rate);
}

void EnemyBase::ApplyPos()
{
	CharacterBase::ApplyPos();
	//歩いて地面から離れたら落下ステートにする//死体(消えている途中も含む)は落下ステートにしない(着地後にIdleへ戻って生き返らないように)
	if (IsLeftFloor() && m_rb.m_vel.y <= 0.0f && !m_isVanishing && !m_isLifeZero)
	{
		ChangeState(std::make_shared<EnemyAirFall>(GetWeakPtr()));
	}
}

Vector3 EnemyBase::TargetPlayerPos()
{
	if (auto player = m_player.lock())
	{
		return player->GetRigidBody().GetPos();
	}

	return Vector3();
}

bool EnemyBase::ChaseTarget(Vector3 target, float distance, float speed)
{
	//プレイヤーの位置に向かって移動する//Y軸は移動しない
	target.y = 0.0f;
	//プレイヤーの手前側が目的地になるように移動//内積は今回はしない
	Vector3 toPlayer = target - m_rb.m_pos;//プレイヤーへのベクトル
	toPlayer.y = 0.0f;

	//playerとの距離を図り、手前側かつ、指定距離まで来たら移動を止める
	if (toPlayer.Magnitude() <= distance)
	{
		//目的地に到達した
		m_rb.m_vel = Vector3(0, 0, 0);
		return true;
	}
	//速度を指定
	m_rb.m_vel = toPlayer.Normalize() * speed;
	//向きを指定
	m_targetVec = toPlayer.Normalize();

	return false;

}

void EnemyBase::CautionMove(Vector3 target, float distance)
{
	//プレイヤーの位置と自分の位置から円を描くように移動する//Y軸は移動しない
	//playerからEnemyへのベクトルの接線方向に移動
	Vector3 toEnemy = m_rb.m_pos - target;
	toEnemy.y = 0.0f;
	//接線の求め方はベクトルを90度回転させるので成分を入れ替えて、xを符号反転　
	Vector3 tangentLine = Vector3(-toEnemy.z, 0.0f, toEnemy.x).Normalize();
	m_rb.m_vel = tangentLine * m_status.cautionSpeed;
	//半径を維持するために後ろ側にもベクトルを加える
	float dist = toEnemy.Magnitude();
	if (dist < distance)
	{
		Vector3 backVec = toEnemy.Normalize() * (distance - dist) * kCautionBackVecRate;//半径を維持するためのベクトル//
		m_rb.m_vel += backVec;
	}
}

bool EnemyBase::BackMove(Vector3 target, float distance)
{
	//ToEnemyの方向に移動する
	Vector3 toEnemy = m_rb.m_pos - target;
	toEnemy.y = 0.0f;
	//到達したら移動を止める
	if (toEnemy.Magnitude() >= distance)
	{
		m_rb.m_vel = Vector3(0, 0, 0);
		return true;
	}

	m_rb.m_vel = toEnemy.Normalize() * m_status.backSpeed;
	return false;
}

int EnemyBase::SelectAttackIndex()
{
	//クールタイム中は攻撃しない
	if (m_attackCoolTime > 0.0f)return -1;

	//プレイヤーとの距離(XZ)
	Vector3 toPlayer = TargetPlayerPos() - m_rb.m_pos;
	toPlayer.y = 0.0f;
	const float distance = toPlayer.Magnitude();

	//距離に入っている攻撃を集める
	std::vector<int> candidates;
	for (const auto& node : m_attackList)
	{
		if (node.index < 0)continue;//CSVで抜けているindex(空の要素)
		if (node.attackRange <= 0.0f)continue;//単体では選ばない攻撃
		if (distance <= node.attackRange)
		{
			candidates.push_back(node.index);
		}
	}
	if (candidates.empty())return -1;

	//ランダムで1つ選ぶ//GetRand(n)は0〜nを返す
	const int pick = GetRand(static_cast<int>(candidates.size()) - 1);
	return candidates[pick];
}

float EnemyBase::GetMaxAttackRange()const
{
	float maxRange = 0.0f;
	for (const auto& node : m_attackList)
	{
		if (node.index < 0)continue;//CSVで抜けているindex(空の要素)
		maxRange = (std::max)(maxRange, node.attackRange);
	}
	return maxRange;
}

bool EnemyBase::CountInterval(float& timer, float interval)
{
	float timeScale = System::GetInstance().GetTimeScale();

	timer += timeScale * m_ownTimeScale;
	if (timer >= interval)
	{
		timer = 0.0f;
		return true;
	}
	return false;
}

void EnemyBase::ToPlayerLook()
{
	auto player = m_player.lock();
	if (!player)return;

	Vector3 toPlayer = player->GetRigidBody().GetPos() - m_rb.m_pos;
	m_targetVec = toPlayer.Normalize();
}

void EnemyBase::FinishHitProcess()
{
	//初期化しておく
	//m_knockBackVel = Vector3(0, 0, 0);
	m_knockBackFrame = 0;
	m_hitType = HitType::None;
}

void EnemyBase::PlayBloodSplash(Collider& other)
{
	auto player = m_player.lock();
	if (!player)return;

	//必殺技中は白い血、それ以外は3パターンからランダム(GetRand(2)は0〜2を返す)
	//必殺技の攻撃が当たった瞬間は、AttackColがこの後にSetUltStartするのでGetIsUltimatingはまだfalse
	//そのため、当たった攻撃が必殺技の判定かどうかも見る
	const bool isUltimating = System::GetInstance().GetBattleMgr()->GetIsUltimating() ||
		other.GetTag().role == Collider::ColRole::UltAttack;
	AsyncData effect = AsyncData::BloodSplashEffectWhite;
	if (!isUltimating)
	{
		const int pattern = GetRand(kBloodSplashPatternNum - 1);
		effect = static_cast<AsyncData>(static_cast<int>(AsyncData::BloodSplashEffectA) + pattern);
	}
	const Vector3 bloodPos(m_rb.m_pos.x, m_rb.m_pos.y + kBloodSplashHeight, m_rb.m_pos.z);
	const int playingHandle = PlayBlood(effect, bloodPos);

	//必殺技中の血は、タイムスケールに関係なくkBloodSplashUltSpeedの速さで飛ばす
	//(当たった直後にスローがかかるので、EffectManagerが毎フレーム速さを設定し直す)
	if (isUltimating)
	{
		EffectManager::GetInstance().SetOwnSpeed(playingHandle, kBloodSplashUltSpeed);
	}
}

int EnemyBase::PlayBlood(AsyncData effect, const Vector3& pos)
{
	auto player = m_player.lock();
	if (!player)return -1;

	//エフェクトの+Zを、プレイヤーから敵へ向かう方向(斬られて血が飛ぶ方向)に向ける
	Vector3 toEnemy = m_rb.m_pos - player->GetRigidBody().GetPos();
	toEnemy.y = 0.0f;
	if (toEnemy.Magnitude() <= 0.0f)
	{
		toEnemy = player->GetTargetVec();//重なっているときはプレイヤーの正面に飛ばす
	}
	return EffectManager::GetInstance().Play(effect, pos, atan2f(toEnemy.x, toEnemy.z), kBloodSplashScale);
}

void EnemyBase::PlayHeadBreakBlood()
{
	if (m_status.headFrame < 0)return;
	const Vector3 neckPos = Vector3::FromDxLibVector(MV1GetFramePosition(m_modelHandle, m_status.headFrame));
	PlayBlood(AsyncData::BloodSplashEffectB, neckPos);
}

void EnemyBase::FinisherPerformanceProcess()
{
	m_isDead = true;
	m_isLifeZero = true;
	//やられ判定を消す
	if (m_hitCol)
	{
		m_hitCol->SetIsActive(false);
	}

}

std::shared_ptr<EnemyStateBase> EnemyBase::NextAfterIdle()
{
	//ランダムでChaseかCautionに遷移する
	const int attackIndex = SelectAttackIndex();
	//攻撃できて攻撃権が取れたら予備動作へ
	auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock();
	if (attackIndex >= 0 && tokenMgr && tokenMgr->Request(*this))
	{
		m_currentAttackIndex = attackIndex;
		return 	std::make_shared<EnemyAttackPre>(GetWeakPtr());
	}
	//ランダム
	//chaseRate(%)の確率でChase、残りはCaution
	if (Utility::IsHitRate(static_cast<int>(m_status.chaseRate)))
	{
		return std::make_shared<EnemyChase>(GetWeakPtr());
	}
	return std::make_shared<EnemyCaution>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> EnemyBase::NextOnArrived()
{
	//攻撃できて攻撃権が取れたら予備動作へ
	const int attackIndex = SelectAttackIndex();
	auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock();
	if (attackIndex >= 0 && tokenMgr && tokenMgr->Request(*this))
	{
		m_currentAttackIndex = attackIndex;
		return std::make_shared<EnemyAttackPre>(GetWeakPtr());
	}
	//目的地に着いたが攻撃できない(クールタイム中など)ので、様子を見ながら待つ
	return std::make_shared<EnemyCaution>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> EnemyBase::NextAfterCaution()
{
	return std::make_shared<EnemyChase>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> EnemyBase::NextAfterAttack()
{
	//後隙
	return std::make_shared<EnemyAttackEnd>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> EnemyBase::NextAfterAttackEnd()
{
	//連撃//今の攻撃にnextAttackがあれば、予備動作なしで次の攻撃へ
	const int next = GetAttackNode(m_currentAttackIndex).nextAttack;
	if (next >= 0)
	{
		assert(next < static_cast<int>(m_attackList.size()) && "nextAttackが攻撃CSVの行数を超えています");
		m_currentAttackIndex = next;
		return std::make_shared<EnemyAttack>(GetWeakPtr(), true);
	}
	//距離を取る
	return std::make_shared<EnemyBack>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> EnemyBase::NextAfterParry()
{
	//弾いた後は反撃する//クールタイムに関係なく出すので固定(仮)
	m_currentAttackIndex = 0;
	return std::make_shared<EnemyAttackPre>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> EnemyBase::NextOnPlayerFound()
{
	return std::make_shared<EnemyChase>(GetWeakPtr());
}

std::shared_ptr<EnemyStateBase> EnemyBase::NextAfterRecover()
{
	//一度Idleに戻り、Idleの待機時間が終わったらNextAfterIdleで次の行動を決める
	return std::make_shared<EnemyIdle>(GetWeakPtr());
}

bool EnemyBase::IsAttackFlowState(const std::shared_ptr<EnemyStateBase>& state) const
{
	return std::dynamic_pointer_cast<EnemyAttackPre>(state) ||
		std::dynamic_pointer_cast<EnemyAttack>(state) ||
		std::dynamic_pointer_cast<EnemyAttackEnd>(state);
}

void EnemyBase::ChangeState(std::shared_ptr<EnemyStateBase> newState)
{
	//攻撃の流れから抜けたら攻撃権を返す(被弾で中断されたときも
	//Initの最初のChangeStateはBattleManagerができる前なので、攻撃の流れから抜けるときだけ見る
	if (IsAttackFlowState(m_currentState) && !IsAttackFlowState(newState))
	{
		if (auto tokenMgr = System::GetInstance().GetBattleMgr()->GetAttackTokenMgr().lock())
		{
			tokenMgr->Release(*this);
		}
	}

	//現在の状態から抜ける
	if (m_currentState)
	{
		m_currentState->Exit();
	}
	//newStateに更新
	m_prevState = m_currentState;
	m_currentState = newState;
	//newStateの初期化
	if (m_currentState)
	{
		m_currentState->Enter();
	}
}

void EnemyBase::InitializeAttackList(const std::vector<std::vector<std::string>>& rawData)
{
	m_attackList.clear();//読み込み直しのときに前のデータが残らないように消す

	for (const auto& tokens : rawData)
	{
		//列数チェック//最後の列(nextAttack)が空欄だと列数が足りなくなるので、使わないときは-1を入れる
		if (tokens.size() < EnemyAttackNodeType::Size)
		{
			assert(false && "敵の攻撃CSVの列数が不足しています");
			continue;
		}
		EnemyAttackNode node;
		node.name = tokens[EnemyAttackNodeType::Name];
		node.index = std::stoi(tokens[EnemyAttackNodeType::Index]);

		//予備動作//再生速度は空欄なら1.0、止めるフレーム・ブレンドは空欄なら-1(デフォルト)
		//アニメが空欄なら予備動作のアニメを流さない、長さ・向く時間は空欄なら0(予備動作なし)
		node.preAnimName = tokens[EnemyAttackNodeType::PreAnimName];
		node.preAnimTimeScale = tokens[EnemyAttackNodeType::PreAnimTimeScale].empty() ? 1.0f : std::stof(tokens[EnemyAttackNodeType::PreAnimTimeScale]);
		node.preAnimEndFrame = tokens[EnemyAttackNodeType::PreAnimEndFrame].empty() ? -1.0f : std::stof(tokens[EnemyAttackNodeType::PreAnimEndFrame]);
		node.preBlendFrame = tokens[EnemyAttackNodeType::PreBlendFrame].empty() ? -1.0f : std::stof(tokens[EnemyAttackNodeType::PreBlendFrame]);
		node.preTime = tokens[EnemyAttackNodeType::PreTime].empty() ? 0.0f : std::stof(tokens[EnemyAttackNodeType::PreTime]);
		node.preLookTime = tokens[EnemyAttackNodeType::PreLookTime].empty() ? 0.0f : std::stof(tokens[EnemyAttackNodeType::PreLookTime]);

		//攻撃//予備動作と同じく空欄なら初期値
		node.animName = tokens[EnemyAttackNodeType::AnimName];
		node.animTimeScale = tokens[EnemyAttackNodeType::AnimTimeScale].empty() ? 1.0f : std::stof(tokens[EnemyAttackNodeType::AnimTimeScale]);
		node.animEndFrame = tokens[EnemyAttackNodeType::AnimEndFrame].empty() ? -1.0f : std::stof(tokens[EnemyAttackNodeType::AnimEndFrame]);
		node.attackFinishFrame = tokens[EnemyAttackNodeType::AttackFinishFrame].empty() ? -1.0f : std::stof(tokens[EnemyAttackNodeType::AttackFinishFrame]);
		node.blendFrame = tokens[EnemyAttackNodeType::BlendFrame].empty() ? -1.0f : std::stof(tokens[EnemyAttackNodeType::BlendFrame]);
		node.attackColStartFrame = std::stof(tokens[EnemyAttackNodeType::AttackColStartFrame]);
		node.attackColEndFrame = std::stof(tokens[EnemyAttackNodeType::AttackColEndFrame]);
		node.moveSpeedBeforeCol = std::stof(tokens[EnemyAttackNodeType::MoveSpeedBeforeCol]);
		node.moveSpeedDuringCol = std::stof(tokens[EnemyAttackNodeType::MoveSpeedDuringCol]);
		node.attackRange = std::stof(tokens[EnemyAttackNodeType::AttackRange]);

		//攻撃のデータ
		node.attackPower = std::stof(tokens[EnemyAttackNodeType::AttackPower]);
		node.brokenRate = std::stof(tokens[EnemyAttackNodeType::BrokenRate]);
		node.knockBackXZ = std::stof(tokens[EnemyAttackNodeType::KnockBackXZ]);
		node.knockBackY = std::stof(tokens[EnemyAttackNodeType::KnockBackY]);
		node.isKirimomi = tokens[EnemyAttackNodeType::IsKirimomi] == "1" ? true : false;//CSVの値が1ならtrue、0ならfalse
		node.hitStopTime = std::stof(tokens[EnemyAttackNodeType::HitStopTime]);

		//当たり判定
		node.colRadius = std::stof(tokens[EnemyAttackNodeType::ColRadius]);
		node.colForwardOffset = std::stof(tokens[EnemyAttackNodeType::ColForwardOffset]);
		node.colOffsetY = std::stof(tokens[EnemyAttackNodeType::ColOffsetY]);

		//攻撃の後
		node.attackEndTimeFrame = std::stof(tokens[EnemyAttackNodeType::AttackEndTimeFrame]);
		node.coolTime = std::stof(tokens[EnemyAttackNodeType::CoolTime]);

		//SE//名前が空ならなし、鳴らす進行度は空欄なら-1(鳴らさない)
		node.seName = tokens[EnemyAttackNodeType::SeName];
		node.seFrameRate = tokens[EnemyAttackNodeType::SeFrameRate].empty() ? -1.0f : std::stof(tokens[EnemyAttackNodeType::SeFrameRate]);

		//連撃//-1なら連撃しない
		node.nextAttack = std::stoi(tokens[EnemyAttackNodeType::NextAttack]);

		//要素の番号=indexなので、CSVはindexの順番に並べる
		assert(node.index == static_cast<int>(m_attackList.size()) && "敵の攻撃CSVのindexが順番になっていません");
		m_attackList.push_back(node);
	}
}

void EnemyBase::InitializeStatus(const std::vector<std::vector<std::string>>& rawData, const std::string& type)
{
	for (const auto& tokens : rawData)
	{
		//列数チェック
		if (tokens.size() < EnemyStatusType::Size)
		{
			assert(false && "EnemyStatus.csvの列数が不足しています");
			continue;
		}
		//自分の種類の行だけ読む
		if (tokens[EnemyStatusType::Type] != type)continue;

		m_status.type = tokens[EnemyStatusType::Type];

		//基本
		m_status.hp = std::stoi(tokens[EnemyStatusType::Hp]);
		m_status.moveSpeed = std::stof(tokens[EnemyStatusType::MoveSpeed]);
		m_status.backSpeed = std::stof(tokens[EnemyStatusType::BackSpeed]);
		m_status.patrolSpeed = std::stof(tokens[EnemyStatusType::PatrolSpeed]);
		m_status.cautionSpeed = std::stof(tokens[EnemyStatusType::CautionSpeed]);

		//当たり判定
		m_status.bodyColOffsetY = std::stof(tokens[EnemyStatusType::BodyColOffsetY]);
		m_status.bodyColHeight = std::stof(tokens[EnemyStatusType::BodyColHeight]);
		m_status.bodyColRadius = std::stof(tokens[EnemyStatusType::BodyColRadius]);
		m_status.hitColOffsetY = std::stof(tokens[EnemyStatusType::HitColOffsetY]);
		m_status.hitColRadius = std::stof(tokens[EnemyStatusType::HitColRadius]);

		//ガード
		m_status.guardRate = std::stof(tokens[EnemyStatusType::GuardRate]);
		m_status.guardTime = std::stof(tokens[EnemyStatusType::GuardTime]);
		m_status.parryGauge = std::stof(tokens[EnemyStatusType::ParryGauge]);
		m_status.parryTime = std::stof(tokens[EnemyStatusType::ParryTime]);
		m_status.guardBreakTime = std::stof(tokens[EnemyStatusType::GuardBreakTime]);
		m_status.forceHitCount = std::stoi(tokens[EnemyStatusType::ForceHitCount]);
		m_status.forceHitResetTime = std::stof(tokens[EnemyStatusType::ForceHitResetTime]);

		//部位破壊
		m_status.partBreakRate = std::stof(tokens[EnemyStatusType::PartBreakRate]);

		//AIの判断
		m_status.backDistance = std::stof(tokens[EnemyStatusType::BackDistance]);
		m_status.chaseRate = std::stof(tokens[EnemyStatusType::ChaseRate]);
		m_status.idleTime = std::stof(tokens[EnemyStatusType::IdleTime]);
		m_status.cautionTime = std::stof(tokens[EnemyStatusType::CautionTime]);
		m_status.chaseTargetUpdateTime = std::stof(tokens[EnemyStatusType::ChaseTargetUpdateTime]);

		//索敵
		m_status.viewLength = std::stof(tokens[EnemyStatusType::ViewLength]);
		m_status.viewHalfAngleDeg = std::stof(tokens[EnemyStatusType::ViewHalfAngleDeg]);

		//被弾
		m_status.knockDownTime = std::stof(tokens[EnemyStatusType::KnockDownTime]);
		m_status.knockBackTime = std::stof(tokens[EnemyStatusType::KnockBackTime]);

		//見た目(部位破壊)
		m_status.leftArmFrame = std::stoi(tokens[EnemyStatusType::LeftArmFrame]);
		m_status.headFrame = std::stoi(tokens[EnemyStatusType::HeadFrame]);
		m_status.leftArmModelScale = std::stof(tokens[EnemyStatusType::LeftArmModelScale]);
		m_status.headModelScale = std::stof(tokens[EnemyStatusType::HeadModelScale]);
		return;
	}
	assert(false && "EnemyStatus.csvに自分の種類の行がありません");
}
