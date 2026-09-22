#include "PlayerStateAttack.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../AttackCol.h"
#include "../../../Collider/SphereShape.h"
#include "../../../Managers/CollisionManager.h"
#include "../System.h"
#include "../../../Camera/CameraManager.h"
#include "../../../Camera/LockOnManager.h"
#include "../../Enemy/EnemyBase.h"
//#include "../../Enemy/EnemyManager.h"
//#include "../Effect/EffectManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	constexpr float kComboInputStart = 0.2f;//コンボ入力受付開始のアニメーションの進行率
	constexpr float kComboInputEnd = 0.8f;//コンボ入力受付終了のアニメーションの進行率

	constexpr float kPlayerCenter = 100.0f;//プレイヤーの当たり判定の中心点までのy軸の距離

	constexpr float kEffectTriggerTime = 0.2f;//エフェクトを出すタイミング
	constexpr float kEffect2TriggerTime = 0.4f;//エフェクトを出すタイミング

	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間
	constexpr float kAttackColOffset = 30.0f;//攻撃の当たり判定を前に出す距離
	constexpr float kAttackColRadius = 150.0f;//攻撃の当たり判定の半径

	const Vector3 kDropAttackKnockBack = Vector3(20.0f, 20.0f, 0.0f);//ドロップ攻撃着地時のノックバック量
	const Vector3 kDropAttackColOffset = Vector3(0.0f, 50.0f, 0.0f);//ドロップ攻撃着地時の当たり判定オフセット
	constexpr float kDropAttackColLifeTime = 10.0f;//ドロップ攻撃着地時の当たり判定の生存時間

	const Vector3 kSkillEffectOffset = Vector3(0.0f, 100.0f, 0.0f);//スキル攻撃エフェクトのオフセット

	constexpr float kEnemyTargetConeAngle = DX_PI_F / 3.0f;//入力方向にいる敵をターゲットにする角度範囲(60度)
	constexpr float kNearbyEnemyRangeMultiplier = 2.0f;//近くの敵を集める範囲(ロックオン範囲の倍率)

	constexpr int kSkillAttackGaugeCost = 20;//スキル攻撃に移行/コンボする際に消費するスキルゲージ量
}


PlayerStateAttack::PlayerStateAttack(std::weak_ptr<Player> player, AttackType type) :
	PlayerState(player), m_attackType(type)
{
	//playerが既に破棄されていたら早期リターンする//trueで破棄されている
	if (m_owner.expired())return;
}

PlayerStateAttack::~PlayerStateAttack()
{
}

void PlayerStateAttack::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//攻撃の方向を決める
	DetermineAttackDirection();
	//ホーミングする対象を初期化
	m_homingEnemyTarget = {};
	//ロックオン中の攻撃の方向を決める
	LockOnAttackDirection();
	//ロックオンしていないとき、入力方向に敵がいたらそいつをターゲットにする
	CheckNoLockOnTargetEnemy();
	//ロックオンしていないときの攻撃の方向を決める//内部ターゲット
	NoLockOnAttackDirection();

	//アニメーションの初期化//コンボの段数によってアニメーションを変える//-1はplayerがいないとき
	int currentComboIndex = SelectAnimInit();
	const ComboNode& node = player->m_comboChain[currentComboIndex];
	//モデルハンドルの取得//攻撃モデルか通常モデルかで切り替える
	int modelHandle = (node.modelType == 0) ? player->m_modelHandle : player->m_attackModelHandle;
	player->m_anim.ChangeAnimWithModelHandle(modelHandle, node.animName, false, 0.8f, node.endFrame);

	//切り上げ攻撃の時は、足元にエフェクトを出す(座標更新は不要、出すだけでいい)
	if (node.index == ComboIndex::upAttack)
	{
		//EffectManager::GetInstance().Play(AsyncData::JumpAttackFootEffect, player->m_rb.m_pos);
	}

	//上下差がある攻撃の時はここで初速を与える
	if (node.moveSpeedY != 0)
	{
		player->m_rb.m_vel = player->m_targetVec * node.moveSpeedX + Vector3(0, node.moveSpeedY, 0);
		//上下の速度を保存
		m_InitVel = player->m_rb.m_vel;
		//上昇攻撃
		if (node.moveSpeedY > 0)
		{
			player->m_isGround = false;//ジャンプ状態にする
			player->SetIsFloor(false);
		}
	}

	float totalAnimFrame = player->m_anim.GetAnimTotalFrame(node.animName);
	//ここでColliderを生成する//あとhitstopとkAttackColOffset
	player->m_attackData = {
	.attackPower = node.attackPower,
	.knockBackPower = Vector3(node.knockBackXZ, node.knockBackY,0),
	//.knockBackPower = Vector3(0.0f,node.knockBackY,0.0f),//吹き飛ばない攻撃にする
	.knockBackFrame = totalAnimFrame,
	.hitStopTime = kHitStopTime,
	.kAttackColOffset = kAttackColOffset,
	.isKirimomi = node.isKirimomi
	};

	m_attackCol = std::make_shared<AttackCol>(m_owner, player->m_attackData);
	Vector3 offset = player->m_targetVec * player->m_attackData.kAttackColOffset
		+ Vector3(0, kPlayerCenter, 0);//プレイヤーの前方に50.0f、y軸方向にkPlayerCenterだけオフセットする

	m_attackCol->ColInit({
		.pos = player->m_rb.m_pos,
		.offset = offset,
		.shape = std::make_unique<SphereShape>(kAttackColRadius),
		.tag = {Collider::Faction::Player, Collider::ColRole::Attack},
		.isActive = false,
		.isTrigger = true
		});//攻撃の当たり判定を初期化する//最初は無効にしておく
	m_attackCol->ResetID(player->GetId());
	m_attackCol->SetIsActive(false);//最初は当たり判定を無効にしておく

	m_isSwingSePlayed = false;//振りのSEをまだ再生していない状態にする
}

void PlayerStateAttack::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//攻撃中の移動処理
	AttackMoveMent();
	//エフェクトを出す
	EffectCheck();
	//振りのSEを出す
	SwingSeCheck();


	//コンボ予約の入力を取る//予約を取ったらもうここは通らないようにする
	AttackInputCheck();
	//攻撃の進行率によってジャンプの入力を受け付けるかどうかを決める
	//ジャンプ
	if (input.IsTriggered("A"))
	{
		if (player->IsFloor())
		{
			AttackFinishProcess();//攻撃の段数を初期化するなどの処理
			player->ChangeState(std::make_shared<PlayerStateJump>(m_owner));
			return;
		}

	}
	float animRate = player->m_anim.GetAnimRate();
	//コンボに移行
	if (animRate >= 0.5f)
	{
		//通常攻撃からスキル攻撃に移行するとき//コンボではなく、スキル攻撃を初めて降ったというシステム
		if (m_isSkillAttackReserved)
		{
			m_isSkillAttackReserved = false;//スキル攻撃の予約を解除する
			AttackFinishProcess();//攻撃の段数を初期化するなどの処理
			//スキルゲージを減らす
			player->AddSkillGauge(-kSkillAttackGaugeCost);

			//スキル攻撃に移行する
			player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::SkillAttack));
			return;
		}

		//コンボ入力が予約されているとき
		if (m_isComboInputReserved)
		{
			//次のコンボに移行する
			StartCombo(m_nextComboIndex);//m_nextComboIndexもm_currentComboIndexも更新されている
			player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, AttackType::None));
			return;
		}
	}

	//コンボ予約がなく、アニメーションが終わったとき//落下攻撃ではないとき
	int currentComboIndex = player->m_comboInfo.currentComboIndex;
	const ComboNode& node = player->m_comboChain[currentComboIndex];
	if (player->m_anim.GetAnimEndFlag() && node.moveSpeedY >= 0)
	{
		//攻撃が終了したら、Idle状態に遷移する//コンボインデックスを初期化
		AttackFinishProcess();
		if (player->IsFloor())
		{
			if (input.IsLeftStickInput())
			{
				//入力があればMove状態に遷移する
				player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
				return;
			}
			else
			{
				player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
				return;
			}
		}
		else//空中は落下状態に移行
		{
			player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));
			return;
		}
	}
	else if (node.moveSpeedY < 0)//落下攻撃の時は、地面と当たるまで//一旦地面に当たるまで
	{
		if (player->IsFloor())//地面と当たったとき
		{
			player->m_isGround = true;//地面にいる状態にする
			m_attackCol->SetIsActive(false);//攻撃の当たり判定を無効にする
			player->m_rb.m_vel = Vector3(0, 0, 0);//突進が終わったら、速度を0にする
			//player->m_hitCol
			//攻撃判定を生成
			InpuctAttackSetUp();
			AttackFinishProcess();
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
			return;
		}
	}

	//アニメーションの更新
	player->m_anim.Update();
}

void PlayerStateAttack::Exit()
{
	//攻撃の当たり判定を削除する//
	if (m_attackCol)
	{
		CollisionManager::GetInstance().ReleaseCollider(m_attackCol);//当たり判定を削除する
		m_attackCol->SetIsActive(false);
		m_attackCol->SetLifeTimeLimited();
		m_attackCol.reset();
	}
}

void PlayerStateAttack::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Attack");
	DrawFormatString(10, 30, GetColor(255, 255, 255), "ComboIndex:%d", m_owner.lock()->m_comboInfo.currentComboIndex);
	DrawFormatString(10, 50, GetColor(255, 255, 255), "m_attackCol Active:%d", m_attackCol->IsActive());
	//isHitの表示
	DrawFormatString(10, 70, GetColor(255, 255, 255), "isHit:%d", m_owner.lock()->m_comboInfo.isHit);
}

void PlayerStateAttack::AttackMoveMent()
{
	auto player = m_owner.lock();
	if (!player) return;

	//ラストヒットの演出用//当たり判定を消す//動きもしない
	if (System::GetInstance().GetIsLastHitEventPlaying())
	{
		m_attackCol->SetIsActive(false);
		return;
	}


	int currentComboIndex = player->m_comboInfo.currentComboIndex;
	const ComboNode& node = player->m_comboChain[currentComboIndex];
	float rate = player->m_anim.GetAnimRate();//アニメーションの進行率を取得

	//上下差がない攻撃とある攻撃で処理を分ける//moveSpeedYが0のときは、上下差がない攻撃とする
	if (node.moveSpeedY == 0.0f)
	{
		//攻撃判定//いったん
		if (rate > node.attackColStartRate && rate < node.attackColEndRate)
		{
			m_attackCol->SetIsActive(true);//攻撃の当たり判定を有効にする
		}
		else
		{
			m_attackCol->SetIsActive(false);//攻撃の当たり判定を無効にする
		}

		//ラストヒットの演出用//当たり判定を消す(下のisHitのreturnより前で必ず通しておく)
		if (System::GetInstance().GetIsLastHitEventPlaying())
		{
			m_attackCol->SetIsActive(false);
		}

		//攻撃が当たったときは、動きを止める
		if (player->m_comboInfo.isHit)
		{
			player->m_rb.m_vel = Vector3(0, 0, 0);//攻撃が当たったときは、速度を0にする
			return;
		}


		//コンボノードで設定された時間内だけ突進
		if (rate < node.moveFrame)
		{
			player->m_rb.m_vel = player->m_targetVec * node.moveSpeedX;//攻撃の最初の数秒は前に突進する
		}
		else
		{
			player->m_rb.m_vel = Vector3(0, 0, 0);//突進が終わったら、速度を0にする
		}
	}
	else//上下差あり
	{
		float timeScale = System::GetInstance().GetTimeScale();
		//重力
		player->m_accumulatedGravity += -Game::kGravity * timeScale * player->m_ownTimeScale;
		player->m_rb.m_vel = m_InitVel + Vector3(0, player->m_accumulatedGravity, 0);
		//player->m_rb.m_vel += Vector3(0, -Game::kGravity, 0) * timeScale;

		//下方向は時間なし//上方向は時間制限あり
		if (node.moveSpeedY > 0)//上向き
		{
			//床から離れる
			player->SetIsFloor(false);
			if (player->m_rb.m_vel.y <= 0)//速度が0になったら上昇終了
			{
				//player->m_rb.m_vel = player->m_targetVec * node.moveSpeedX + Vector3(0, 0, 0);
				player->m_rb.m_vel = Vector3(0, 0, 0);//終わったら、速度を0にする
				m_attackCol->SetIsActive(false);//攻撃の当たり判定を無効にする
			}
			//まだ上昇中
			else
			{
				//判定を有効
				m_attackCol->SetIsActive(true);//攻撃の当たり判定
			}
		}
		else//下向き//常に下方向の速度を与える
		{
			//判定を有効
			m_attackCol->SetIsActive(true);//攻撃の当たり判定
		}


	}

	

}

void PlayerStateAttack::DetermineAttackDirection()
{
	auto player = m_owner.lock();
	if (!player) return;

	auto& input = Input::GetInstance();
	Vector3 attackDir = Vector3(0, 0, 0);

	//攻撃の方向を決める//カメラの向きと入力から、方向を決める
	if (input.IsPressed("Up"))
	{
		attackDir += player->forward;
	}
	if (input.IsPressed("Down"))
	{
		attackDir += player->down;
	}
	if (input.IsPressed("Left"))
	{
		attackDir += player->left;
	}
	if (input.IsPressed("Right"))
	{
		attackDir += player->right;
	}
	//入力がないときは、playerの向いている方向に進む//あるときはその方向に進む//この処理に問題があるらしい
	if (attackDir.Magnitude() <= 0.0f)
	{
		player->m_targetVec = player->m_targetVec.Normalize();

		//player->m_targetVec = player->forward.Normalize();
	}
	else
	{
		player->m_targetVec = attackDir.Normalize();
	}

}

void PlayerStateAttack::LockOnAttackDirection()
{
	auto player = m_owner.lock();
	if (!player) return;
	//ロックオンしているかどうか
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;
	//メインカメラを取得
	//auto mainCamera = cameraManager->GetMainCamera();

	//bool isLockOn = mainCamera->GetIsLockOn();
	//ロックオンしていないならreturnする
	//if (!isLockOn)return;

	//ターゲットしている敵を取得
	auto lockedEnemy = cameraManager->GetTargetEnemy();
	if (!lockedEnemy)return;
	//ターゲットしている敵が死んでいるならreturnする
	if (lockedEnemy->GetIsLifeZero())return;
	//ターゲットしている敵の方向にプレイヤーを向く
	Vector3 enemyPos = lockedEnemy->GetRigidBody().GetPos();

	Vector3 dirToEnemy = (enemyPos - player->m_rb.m_pos).Normalize();
	player->m_targetVec = dirToEnemy;

}

void PlayerStateAttack::NoLockOnAttackDirection()
{
	auto& input = Input::GetInstance();

	auto player = m_owner.lock();
	if (!player) return;
	//ロックオンしているかどうか
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;
	//メインカメラを取得
	//auto mainCamera = cameraManager->GetMainCamera();

	//bool isLockOn = mainCamera->GetIsLockOn();
	//ロックオンしていたらreturnする
	//if (isLockOn)return;
	//ターゲットしている敵を取得
	if (m_homingEnemyTarget.lock())
	{
		//ターゲットしている敵が死んでいるならreturnする
		if (m_homingEnemyTarget.lock()->GetIsLifeZero())return;

		Vector3 enemyPos = m_homingEnemyTarget.lock()->GetRigidBody().GetPos();
		Vector3 playerPos = player->m_rb.m_pos;
		enemyPos.y = playerPos.y = 0;//y軸方向は無視する//XZ平面での角度を計算する

		Vector3 dirToEnemy = (enemyPos - playerPos).Normalize();
		player->m_targetVec = dirToEnemy;
	}
	//ホーミングする対象がいない場合は、そのままスティック入力

	//入力がないなら//インターンで得た情報
	//ターゲットしている敵の方向にプレイヤーを向く
	
	
}

void PlayerStateAttack::CheckNoLockOnTargetEnemy()
{
	//auto player = m_owner.lock();
	//if (!player) return;
	//auto& input = Input::GetInstance();

	////ロックオンしているかどうか
	//auto cameraManager = player->m_cameraManager.lock();
	//if (!cameraManager)return;
	////メインカメラを取得
	//auto mainCamera = cameraManager->GetMainCamera();

	//bool isLockOn = mainCamera->GetIsLockOn();
	////ロックオンしていたらreturnする
	//if (isLockOn)return;

	////プレイヤーの一定範囲内にいる敵を取得
	//auto enemyManager = player->m_enemyManager.lock();
	//if (!enemyManager)return;
	//auto enemies = enemyManager->GetEnemies();
	//std::vector<std::shared_ptr<EnemyBase>> nearbyEnemies;
	////playerが空中にいるなら空中の敵の身を取得
	//bool isPlayerAir = !player->IsFloor();
	//for (auto& enemy : enemies)
	//{
	//	if (enemy->GetIsLifeZero()) continue;
	//	//playerが空中
	//	if (isPlayerAir)
	//	{
	//		if (enemy->IsFloor()) continue;
	//	}
	//	//playerが地上
	//	else
	//	{
	//		if (!enemy->IsFloor()) continue;
	//	}

	//	//範囲内にいる敵を集める
	//	Vector3 enemyPos = enemy->GetPos();
	//	float distance = (enemyPos - player->m_rb.m_pos).Magnitude();

	//	if(distance < player->GetCameraRockOnRange() * kNearbyEnemyRangeMultiplier)
	//	{
	//		nearbyEnemies.push_back(enemy);
	//	}
	//}
	////近くに敵がいなかったらreturnする
	//if (nearbyEnemies.empty()) return;

	////入力方向にベクトルを飛ばし、そこと、cosΘで比較
	////30度以内の敵がいたら、そいつをターゲットにする
	//Vector3 inputDir = Vector3(0, 0, 0);
	//float cosTheta = cosf(kEnemyTargetConeAngle);//角度以内の敵をターゲットにする//cosでの判定に使う

	//if (input.IsPressed("Up")) inputDir += player->forward;
	//if (input.IsPressed("Down")) inputDir += player->down;
	//if (input.IsPressed("Left")) inputDir += player->left;
	//if (input.IsPressed("Right")) inputDir += player->right;

	//if (inputDir.Magnitude() <= 0.0f)
	//{
	//	//入力がないときは、playerの向いている方向を入力方向とする
	//	//→入力がないとき、かつ内部ターゲットがいないとき
	//	if (!player->m_lockOnManager.lock()->GetTarget().lock())
	//	{
	//		inputDir = player->m_targetVec.Normalize();
	//		//入力がないとき、かつ、敵ターゲットがまだいないときはカメラのベクトルにする
	//		//そして、ターゲットを探す
	//		if (!cameraManager->GetTargetEnemy())
	//		{
	//			Vector3 cameraPos = mainCamera->GetCameraPos();
	//			Vector3 playerPos = player->m_rb.m_pos;
	//			cameraPos.y = playerPos.y = 0;//y軸方向は無視する//XZ平面での角度を計算する
	//			inputDir = (playerPos - cameraPos).Normalize();
	//			//cosを広げる
	//			cosTheta = cosf(kEnemyTargetConeAngle);//60度以内の敵をターゲットにする//cosでの判定に使う
	//		}
	//	}
	//	else
	//	{
	//		//入力がないかつ内部ターゲットがいるときはそいつの方向に向かわせる
	//		//吸い寄せ対象をセット
	//		
	//		m_homingEnemyTarget = player->m_lockOnManager.lock()->GetTarget();
	//		return;
	//	}
	//	
	//}
	////入力があるとき
	//else
	//{
	//	//入力方向から探す
	//	inputDir = inputDir.Normalize();
	//}

	////cosが最大の敵をターゲットにする//-1.0fで初期化
	//float MaxCos = -1.0f;
	//
	//std::shared_ptr<EnemyBase> bestTarget = nullptr;
	//for(auto& enemy : nearbyEnemies)
	//{
	//	Vector3 enemyPos = enemy->GetPos();
	//	Vector3 playerPos = player->m_rb.m_pos;
	//	enemyPos.y = playerPos.y;//y軸方向は無視する//XZ平面での角度を計算する
	//	Vector3 dirToEnemy = (enemyPos - player->m_rb.m_pos).Normalize();
	//	float cos = inputDir.Dot(dirToEnemy);
	//	if (cos < cosTheta) continue;//30度以内の敵じゃなかったらスキップ
	//	if(cos > MaxCos)
	//	{
	//		MaxCos = cos;
	//		bestTarget = enemy;
	//	}
	//}
	////ターゲットが見つかったら、カメラにセットする
	//if(bestTarget)
	//{
	//	/*auto lockOnManager = player->m_lockOnManager.lock();
	//	lockOnManager->SetTargetEnemy(bestTarget->GetId());*/
	//	//吸い寄せ対象をセット
	//	m_homingEnemyTarget = bestTarget;
	//}

}

void PlayerStateAttack::AttackInputCheck()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();
	//既に予約があるならリターン
	if (m_isComboInputReserved) return;

	//攻撃の入力を受け付けるか
	float animRate = player->m_anim.GetAnimRate();
	bool canInput = animRate >= kComboInputStart && animRate <= kComboInputEnd;//コンボ入力受付時間内かどうか//0.2から0.8の間
	if (!canInput)return;//コンボ入力受付時間外なら、ここで処理を終える

	//現在のコンボノードを取得
	int currentComboIndex = player->m_comboInfo.currentComboIndex;
	const ComboNode& currentNode = player->m_comboChain[currentComboIndex];
	//スキル攻撃かどうか
	bool isSkillAttack = currentNode.index == ComboIndex::SkillAttack1 ||
		currentNode.index == ComboIndex::SkillAttack2 ||
		currentNode.index == ComboIndex::SkillAttack3;
	bool isPlayerAir = !player->IsFloor();//空中にいるかどうか//空中にいるときは、空中攻撃に移行する
	bool WasSkillAirAttack = player->m_comboInfo.isAirSkillAttack;//空中でスキル攻撃をしたかどうか
	bool WasAirAttack = player->m_comboInfo.isAirAttack;//空中で攻撃をしたかどうか

	//スキル攻撃
	if (input.IsPressed("LB") && input.IsTriggered("X"))
	{
		//スキル攻撃ができるかどうか
		if (player->CanSkillAttack(false))
		{
			//通常攻撃ならば、攻撃を終了して、スキル攻撃に移行
			//スキル攻撃ならばコンボ攻撃に移行
			if (isSkillAttack)
			{
				//コンボ攻撃に移行する
				//弱攻撃ボタンでつながる次のコンボがあるか
				if (!currentNode.nextWeakAttack.empty())//空じゃなかったら
				{
					m_nextComboIndex = currentNode.nextWeakAttack[0];//次のコンボ番号をセットする//今回は1つしかないので、0番目をセットする
					m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
				}
			}
			//コンボ処理を終了して、スキル攻撃に移行する
			else
			{
				//空中でスキル攻撃を行っていたらスキル攻撃に移行しない
				if (isPlayerAir && WasSkillAirAttack)return;

				m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
				m_isSkillAttackReserved = true;//スキル攻撃の予約がされているフラグを立てる
			}
		}
	}
	//強攻撃
	else if (!input.IsPressed("LB") && input.IsTriggered("Y"))
	{
		//スキルアタックだったら弱攻撃につなぐ
		if (isSkillAttack)
		{
			//地上だったら弱攻撃1に移行する//空中だったら空中弱攻撃に移行する
			if (player->IsFloor())
			{
				m_nextComboIndex = ComboIndex::HeavyAttack1;//強攻撃1に移行する
				m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
			}
			else
			{
				m_nextComboIndex = ComboIndex::AirHeavyAttack1;//空中強攻撃に移行する
				m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
			}

		}

		if (isSkillAttack)return;
		if (!currentNode.nextHeavyAttack.empty())//空じゃなかったら
		{
			m_nextComboIndex = currentNode.nextHeavyAttack[0];//次のコンボ番号をセットする//今回は1つしかないので、0番目をセットする
			m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
		}
	}
	//弱攻撃
	else if (!input.IsPressed("LB") && input.IsTriggered("X"))
	{
		//スキルアタックだったら弱攻撃につなぐ
		if (isSkillAttack)
		{
			//地上だったら弱攻撃1に移行する//空中だったら空中弱攻撃に移行する
			if (player->IsFloor())
			{
				m_nextComboIndex = ComboIndex::LightAttack1;//弱攻撃1に移行する
				m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
			}
			else
			{
				//初めての弱攻撃ならする
				if (!player->m_comboInfo.isAirAttack)
				{
					m_nextComboIndex = ComboIndex::AirAttack1;//空中弱攻撃に移行する
					m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
				}
			}

		}

		if (isSkillAttack)return;
		if (!currentNode.nextWeakAttack.empty())//空じゃなかったら
		{
			m_nextComboIndex = currentNode.nextWeakAttack[0];//次のコンボ番号をセットする//今回は1つしかないので、0番目をセットする
			m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
		}

	}
	//else if (!input.IsPressed("LB") && input.IsTriggered("Y"))
	//{
	//	//強攻撃ボタンでつながる次のコンボがあるか
	//	if (!currentNode.nextHeavyAttack.empty())//空じゃなかったら
	//	{
	//		m_nextComboIndex = currentNode.nextHeavyAttack[0];//次のコンボ番号をセットする//今回は1つしかないので、0番目をセットする
	//		m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
	//	}
	//}
}

void PlayerStateAttack::StartCombo(int comboIndex)
{
	auto player = m_owner.lock();
	if (!player) return;
	//範囲外だったら早期リターン
	if (comboIndex < 0 || comboIndex >= player->m_comboChain.size())return;
	//isHitをfalseにする
	player->m_comboInfo.isHit = false;
	//鴉状態を解除する
	player->m_isRaven = false;
	//現在のコンボの段数を更新
	player->m_comboInfo.currentComboIndex = comboIndex;
	m_isComboInputReserved = false;//コンボ入力の予約を解除する

	//Skill攻撃2,3だったらスキルゲージを減らす
	int currentComboIndex = player->m_comboInfo.currentComboIndex;
	bool isSkillAttack2or3 =
		currentComboIndex == ComboIndex::SkillAttack2 ||
		currentComboIndex == ComboIndex::SkillAttack3;

	if (isSkillAttack2or3)
	{
		player->AddSkillGauge(-kSkillAttackGaugeCost);
	}
}

void PlayerStateAttack::AttackFinishProcess()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->m_comboInfo.currentComboIndex = ComboIndex::None;//攻撃していない状態に戻す
	player->m_comboInfo.isHit = false;//攻撃が当たったかどうか
	//y軸の速度を0に戻す
	player->m_rb.m_vel.y = 0.0f;
	//鴉状態を解除する
	player->m_isRaven = false;

	//攻撃の当たり判定の開放
	//m_attackCol->SetIsActive(false);
	//m_attackCol->SetLifeTimeLimited();
	//m_attackCol.reset();
}

int PlayerStateAttack::SelectAnimInit()
{
	auto player = m_owner.lock();
	if (!player)return -1;

	//アニメーションの初期化
	int currentComboIndex = player->m_comboInfo.currentComboIndex;//現在のコンボインデックスを取得する
	if (currentComboIndex == ComboIndex::None)//コンボの段数が-1のときは、最初のコンボを再生する
	{


		if (m_attackType == AttackType::lightAttack)
		{
			if (player->IsFloor())
			{
				currentComboIndex = ComboIndex::LightAttack1;//弱攻撃の最初の段数を0に設定する
			}
			else
			{
				player->m_comboInfo.isAirAttack = true;//空中攻撃のフラグを立てる
				currentComboIndex = ComboIndex::AirAttack1;//空中攻撃1
			}
		}
		else if (m_attackType == AttackType::heavyAttack)
		{
			if (player->IsFloor())
			{
				currentComboIndex = ComboIndex::HeavyAttack1;//強攻撃の最初の段数を1に設定する//今回は、弱攻撃が0番目、強攻撃が1番目の段数から始まるようにする
			}
			else
			{
				currentComboIndex = ComboIndex::AirHeavyAttack1;//空中強攻撃1
			}
		}
		else if (m_attackType == AttackType::SkillAttack)
		{
			currentComboIndex = ComboIndex::SkillAttack1;
			if (!player->IsFloor())player->m_comboInfo.isAirSkillAttack = true;//空中攻撃のフラグを立てる

		}
		//スキルアタックだったら鴉状態にする
		bool isSkillAttack = currentComboIndex == ComboIndex::SkillAttack1 ||
			currentComboIndex == ComboIndex::SkillAttack2 ||
			currentComboIndex == ComboIndex::SkillAttack3;
		if (isSkillAttack)player->m_isRaven = true;//鴉状態にする
		else player->m_isRaven = false;//鴉状態を解除する
		//現在のコンボの段数を更新する
		player->m_comboInfo.currentComboIndex = currentComboIndex;
	}
	else
	{
		//コンボ攻撃で空中攻撃だった時、AirAttackをtrueにする
		if (currentComboIndex == ComboIndex::AirAttack1)
		{
			player->m_comboInfo.isAirAttack = true;
		}
		//スキルアタックだったら鴉状態にする
		bool isSkillAttack = currentComboIndex == ComboIndex::SkillAttack1 ||
			currentComboIndex == ComboIndex::SkillAttack2 ||
			currentComboIndex == ComboIndex::SkillAttack3;
		if (isSkillAttack)player->m_isRaven = true;//鴉状態にする
		else player->m_isRaven = false;//鴉状態を解除する

		//コンボの段数が-1でないときは、次のコンボを再生する
		//currentComboIndex = m_nextComboIndex;//次のコンボの段数を取得する
		//player->m_comboInfo.currentComboIndex = currentComboIndex;//現在のコンボの段数を更新する
	}
	return currentComboIndex;
}

void PlayerStateAttack::InpuctAttackSetUp()
{
	auto player = m_owner.lock();
	if (!player) return;
	const ComboNode& node = player->m_comboChain[player->m_comboInfo.currentComboIndex];
	float totalAnimFrame = player->m_anim.GetAnimTotalFrame(node.animName);
	//ドロップ攻撃の時は当たり判定を生成
	if (node.moveSpeedY < 0)
	{
		CharacterBase::AttackData dropAttackData = {
			.attackPower = 0.0f,
			.knockBackPower = kDropAttackKnockBack,
			.knockBackFrame = totalAnimFrame,
			.hitStopTime = kHitStopTime,
			.kAttackColOffset = kAttackColOffset,
			.isKirimomi = true
		};
		//AttackColを生成
		player->m_burstAttackCol = std::make_shared<AttackCol>(m_owner, dropAttackData);
		player->m_burstAttackCol->ColInit({
			.pos = player->m_rb.m_pos,
			.offset = kDropAttackColOffset,
			.shape = std::make_unique<SphereShape>(kAttackColRadius),
			.tag = {Collider::Faction::Player, Collider::ColRole::Attack},
			.isActive = true,
			.isTrigger = true,
			.lifeTime = kDropAttackColLifeTime
			});//攻撃の当たり判定を初期化する//最初は無効にしておく
		player->m_burstAttackCol->ResetID(player->GetId());
		player->m_burstAttackCol->SetIsActive(true);//攻撃の当たり判定を有効にする
	}
}

void PlayerStateAttack::EffectCheck()
{
	//auto player = m_owner.lock();
	//if (!player)return;
	//int currentComboIndex = player->m_comboInfo.currentComboIndex;
	//const ComboNode& node = player->m_comboChain[currentComboIndex];
	//float rate = player->m_anim.GetAnimRate();//アニメーションの進行率を取得
	//if (currentComboIndex == ComboIndex::SkillAttack1)
	//{
	//	if (rate >= kEffectTriggerTime)
	//	{
	//		/*player->m_efPlayingHandle = PlayEffekseer3DEffect(player->m_efHandle);
	//		SetPosPlayingEffekseer3DEffect(player->m_efPlayingHandle, player->m_rb.m_pos.x, player->m_rb.m_pos.y+100, player->m_rb.m_pos.z);
	//		SetRotationPlayingEffekseer3DEffect(player->m_efPlayingHandle, 0.0f, player->m_rotAngleY + DX_PI_F, 0.0f);*/
	//		player->m_efPlayingHandle = EffectManager::GetInstance().Play(AsyncData::PlayerEffectSkill, 
	//			player->m_rb.m_pos + kSkillEffectOffset, player->m_rotAngleY + DX_PI_F);
	//	}
	//	//エフェクトが出ているとき
	//	else
	//	{
	//		//座標の更新
	//		EffectManager::GetInstance().SetPos(player->m_efPlayingHandle, player->m_rb.m_pos + kSkillEffectOffset);
	//		EffectManager::GetInstance().SetRot(player->m_efPlayingHandle, player->m_rotAngleY + DX_PI_F);
	//	}
	//}
	//if (currentComboIndex == ComboIndex::SkillAttack2)
	//{
	//	if (rate >= kEffect2TriggerTime)
	//	{
	//	
	//		player->m_efPlayingHandle = EffectManager::GetInstance().Play(AsyncData::PlayerEffectSkill2, 
	//			player->m_rb.m_pos + kSkillEffectOffset, player->m_rotAngleY + DX_PI_F);
	//	}
	//	//エフェクトが出ているとき
	//	else
	//	{
	//		//座標の更新
	//		EffectManager::GetInstance().SetPos(player->m_efPlayingHandle, player->m_rb.m_pos + kSkillEffectOffset);
	//		EffectManager::GetInstance().SetRot(player->m_efPlayingHandle, player->m_rotAngleY + DX_PI_F);
	//	}
	//}
	//if (currentComboIndex == ComboIndex::SkillAttack3)
	//{
	//	if (rate >= kEffect2TriggerTime)
	//	{
	//	
	//		player->m_efPlayingHandle = EffectManager::GetInstance().Play(AsyncData::PlayerEffectSkill3, 
	//			player->m_rb.m_pos + kSkillEffectOffset, player->m_rotAngleY + DX_PI_F);
	//	}
	//	//エフェクトが出ているとき
	//	else
	//	{
	//		//座標の更新
	//		EffectManager::GetInstance().SetPos(player->m_efPlayingHandle, player->m_rb.m_pos + kSkillEffectOffset);
	//		EffectManager::GetInstance().SetRot(player->m_efPlayingHandle, player->m_rotAngleY + DX_PI_F);
	//	}
	//}
}

void PlayerStateAttack::SwingSeCheck()
{
	//auto player = m_owner.lock();
	//if (!player)return;
	//if (m_isSwingSePlayed)return;//既に再生済みなら何もしない

	//int currentComboIndex = player->m_comboInfo.currentComboIndex;
	//const ComboNode& node = player->m_comboChain[currentComboIndex];
	////ComboChain.csvでSE名が指定されていない場合は何もしない
	//if (node.seFrameRate < 0.0f || node.seName.empty())return;

	//float rate = player->m_anim.GetAnimRate();//アニメーションの進行率を取得
	//if (rate < node.seFrameRate)return;

	//System::GetInstance().GetSoundManager().PlaySE(node.seName);
	//m_isSwingSePlayed = true;
}
