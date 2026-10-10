#include "PlayerStateAttackBase.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../AttackCol.h"
#include "../../../Collider/SphereShape.h"
#include "../../../Managers/CollisionManager.h"
#include "../System.h"
#include "../../../BattleManager.h"
#include "../../../Camera/CameraManager.h"
#include "../../../Camera/CameraState/CameraStateBase.h"
#include "../../Enemy/EnemyBase.h"
#include "../../Enemy/EnemyManager.h"

namespace
{
	constexpr float kDefaultMoveEndRate = 0.2f;//突進終了//CSVの遷移フレームが空欄(-1)のときに使うデフォルトの進行率

	constexpr float kPlayerCenter = 100.0f;//プレイヤーの当たり判定の中心点までのy軸の距離

	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間
	constexpr float kAttackColOffset = 30.0f;//攻撃の当たり判定を前に出す距離
	constexpr float kAttackColRadius = 150.0f;//攻撃の当たり判定の半径

	const Vector3 kDropAttackKnockBack = Vector3(20.0f, 20.0f, 0.0f);//ドロップ攻撃着地時のノックバック量
	const Vector3 kDropAttackColOffset = Vector3(0.0f, 50.0f, 0.0f);//ドロップ攻撃着地時の当たり判定オフセット
	constexpr float kDropAttackColLifeTime = 10.0f;//ドロップ攻撃着地時の当たり判定の生存時間

	constexpr float kEnemyTargetConeAngle = DX_PI_F / 3.0f;//入力方向にいる敵をターゲットにする角度範囲(60度)
	constexpr float kNearbyEnemyRangeMultiplier = 2.0f;//近くの敵を集める範囲(ロックオン範囲の倍率)
}

PlayerStateAttackBase::PlayerStateAttackBase(std::weak_ptr<Player> player) :
	PlayerState(player)
{
}

void PlayerStateAttackBase::DecideAttackDirection()
{
	//攻撃の方向を決める
	DetermineAttackDirection();
	//ロックオン中の攻撃の方向を決める
	LockOnAttackDirection();
	//ロックオンしていないとき、入力方向に敵がいたらそいつをターゲットにする
	CheckNoLockOnTargetEnemy();
	//ロックオンしていないときの攻撃の方向を決める//内部ターゲット
	NoLockOnAttackDirection();
}

void PlayerStateAttackBase::DetermineAttackDirection()
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

void PlayerStateAttackBase::LockOnAttackDirection()
{
	auto player = m_owner.lock();
	if (!player) return;
	//ロックオンしているかどうか
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;
	//ロックオンしていないならreturnする
	if (!player->IsLockOn())return;

	//ターゲットしている敵を取得
	auto lockedEnemy = player->GetAttackTarget();
	if (!lockedEnemy)return;
	//ターゲットしている敵が死んでいるならreturnする
	if (lockedEnemy->GetIsLifeZero())return;
	//ターゲットしている敵の方向にプレイヤーを向く
	Vector3 enemyPos = lockedEnemy->GetRigidBody().GetPos();

	Vector3 dirToEnemy = (enemyPos - player->m_rb.m_pos).Normalize();
	dirToEnemy.y = 0.0f;
	player->m_targetVec = dirToEnemy;

}

void PlayerStateAttackBase::NoLockOnAttackDirection()
{
	auto player = m_owner.lock();
	if (!player) return;
	//ロックオンしていたらreturnする
	if (player->IsLockOn())return;

	//内部ターゲットの方向に吸い寄せる//死んでいたらnullptrが返る
	auto softTarget = player->GetSoftTarget();
	if (!softTarget)return;

	Vector3 enemyPos = softTarget->GetRigidBody().GetPos();
	Vector3 playerPos = player->m_rb.m_pos;
	enemyPos.y = playerPos.y = 0;//y軸方向は無視する//XZ平面での角度を計算する
	Vector3 dirToEnemy = (enemyPos - playerPos).Normalize();
	player->m_targetVec = dirToEnemy;
	//内部ターゲットがいない場合は、そのままスティック入力

	//入力がないなら//インターンで得た情報
	//ターゲットしている敵の方向にプレイヤーを向く
}

void PlayerStateAttackBase::CheckNoLockOnTargetEnemy()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//ロックオンしているかどうか
	auto cameraManager = player->m_cameraManager.lock();
	if (!cameraManager)return;

	//ロックオンしていたらreturnする
	if (player->IsLockOn())return;

	////入力方向にベクトルを飛ばし、そこと、cosΘで比較
	////30度以内の敵がいたら、そいつをターゲットにする
	//スティックの入力方向を求める
	Vector3 inputDir = Vector3(0, 0, 0);
	if (input.IsPressed("Up")) inputDir += player->forward;
	if (input.IsPressed("Down")) inputDir += player->down;
	if (input.IsPressed("Left")) inputDir += player->left;
	if (input.IsPressed("Right")) inputDir += player->right;
	bool hasInput = inputDir.Magnitude() > 0.0f;

	//入力がないとき
	//入力がないときは、playerの向いている方向を入力方向とする
	if (!hasInput)
	{
		//内部ターゲットがいれば、そのまま使う
		if (player->GetSoftTarget())return;

		//内部ターゲットがいなければ、カメラの向いている方向から探す
		auto cameraManager = player->m_cameraManager.lock();
		if (!cameraManager)return;
		auto camera = cameraManager->GetActiveCamera();
		if (!camera)return;
		Vector3 cameraPos = camera->GetPos();
		Vector3 playerPos = player->m_rb.m_pos;
		cameraPos.y = playerPos.y = 0.0f;//y軸方向は無視する//XZ平面での角度を計算する
		inputDir = playerPos - cameraPos;
	}
	inputDir.y = 0.0f;
	//カメラが真上にある場合など向きが決まらないときはreturnする
	if (inputDir.Magnitude() <= 0.0f)return;
	inputDir = inputDir.Normalize();

	//プレイヤーの一定範囲内にいる敵を集める
	auto enemyManager = player->m_enemyManager.lock();
	if (!enemyManager)return;
	bool isPlayerAir = !player->IsFloor();
	float range = player->GetCameraRockOnRange() * kNearbyEnemyRangeMultiplier;
	float cosTheta = cosf(kEnemyTargetConeAngle);//この角度以内の敵をターゲットにする//cosでの判定に使う

	//入力方向とのcosが最大の敵をターゲットにする
	std::shared_ptr<EnemyBase> bestTarget = nullptr;
	float maxCos = -1.0f;
	for (auto& enemy : enemyManager->GetEnemies())
	{
		if (!enemy)continue;
		if (enemy->GetIsLifeZero())continue;
		//プレイヤーが空中なら空中の敵だけ、地上なら地上の敵だけを対象にする
		if (isPlayerAir == enemy->IsFloor())continue;

		Vector3 enemyPos = enemy->GetRigidBody().GetPos();
		Vector3 playerPos = player->m_rb.m_pos;
		if ((enemyPos - playerPos).Magnitude() >= range)continue;

		enemyPos.y = playerPos.y;//y軸方向は無視する//XZ平面での角度を計算する
		Vector3 dirToEnemy = (enemyPos - playerPos).Normalize();
		float cos = inputDir.Dot(dirToEnemy);
		if (cos < cosTheta)continue;//角度の範囲外ならスキップ
		if (cos > maxCos)
		{
			maxCos = cos;
			bestTarget = enemy;
		}
	}

	if (bestTarget)
	{
		//吸い寄せ対象
		//見つかったら内部ターゲットにする
		player->SetSoftTarget(bestTarget);
	}
	else if (hasInput)
	{
		//敵のいない方向に入力したときは、前の内部ターゲットを狙わない
		player->ClearSoftTarget();
	}
}

void PlayerStateAttackBase::AttackMoveMent()
{
	auto player = m_owner.lock();
	if (!player) return;

	//ラストヒットの演出用//当たり判定を消す//動きもしない
	if (System::GetInstance().GetBattleMgr()->GetIsLastHitEventPlaying())
	{
		m_attackCol->SetIsActive(false);
		if (m_dropAttackCol)m_dropAttackCol->SetIsActive(false);//下降攻撃の追加ヒットも止める
		return;
	}


	int currentComboIndex = player->m_comboInfo.currentComboIndex;
	const ComboNode& node = player->m_comboChain[currentComboIndex];

	//上下差がない攻撃とある攻撃で処理を分ける//moveSpeedYが0のときは、上下差がない攻撃とする
	if (node.moveSpeedY == 0.0f)
	{
		//攻撃判定//多段ヒットの処理はUpdateAttackWindowにまとめた
		bool isInWindow = UpdateAttackWindow(*m_attackCol, node);
		m_attackCol->SetIsActive(isInWindow);//どこかの区間に入ってたら判定ON

		//ラストヒットの演出用//当たり判定を消す(下のisHitのreturnより前で必ず通しておく)
		if (System::GetInstance().GetBattleMgr()->GetIsLastHitEventPlaying())
		{
			m_attackCol->SetIsActive(false);
		}

		//攻撃が当たったときは、動きを止める
		if (player->m_comboInfo.isHit)
		{
			player->m_rb.m_vel = Vector3(0, 0, 0);//攻撃が当たったときは、速度を0にする
			return;
		}


		//コンボノードで設定されたフレームの間だけ突進
		float moveEndFrame = ResolveTransitionFrame(node.moveEndFrame, kDefaultMoveEndRate);
		if (player->m_anim.IsAnimFrameBetween(node.moveStartFrame, moveEndFrame))
		{
			player->m_rb.m_vel = player->m_targetVec * node.moveSpeedX;//攻撃の最初の数秒は前に突進する
		}
		else
		{
			player->m_rb.m_vel = Vector3(0, 0, 0);//突進が終わったら、速度を0にする
		}
	}
	else//上下差あり//終了は地面につくまで(上昇は速度が0になるまで)なので、moveEndFrameは使わない
	{
		//moveStartFrameまでは動かない(重力も加算しない)
		if (player->m_anim.GetNowAnimFrame() < node.moveStartFrame)
		{
			player->m_rb.m_vel = Vector3(0, 0, 0);
			return;
		}

		//上昇攻撃は、動き出したタイミングで床から離れる
		if (node.moveSpeedY > 0)
		{
			player->m_isGround = false;//ジャンプ状態にする
		}

		float timeScale = System::GetInstance().GetTimeScale();
		//重力
		m_gravity += -Game::kGravity * timeScale * player->m_ownTimeScale;
		player->m_rb.m_vel = m_InitVel + Vector3(0, m_gravity, 0);
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
			//判定を有効//こっちは今まで通り着地までずっとON//1ヒット目で敵をEnemyHitDropに入れる役
			m_attackCol->SetIsActive(true);//攻撃の当たり判定

			//追加のヒットはm_dropAttackColでCSVの区間ごとに出す//ノックバック0なのでダメージだけ
			if (m_dropAttackCol)
			{
				bool isInWindow = UpdateDropAttackWindow(*m_dropAttackCol, node);
				m_dropAttackCol->SetIsActive(isInWindow);
			}
		}


	}

}

bool PlayerStateAttackBase::UpdateAttackWindow(AttackCol& col, const ComboNode& node)
{
	auto player = m_owner.lock();
	if (!player) return false;

	//今どの区間にいるか探す//どこにも入ってなければ-1
	int activeWindow = -1;
	for (int i = 0; i < static_cast<int>(node.attackColStartFrames.size()); ++i)
	{
		if (player->m_anim.IsAnimFrameBetween(node.attackColStartFrames[i], node.attackColEndFrames[i]))
		{
			activeWindow = i;
			break;
		}
	}

	//新しい区間に入った1フレームだけ通る
	if (activeWindow != -1 && activeWindow != m_lastAttackWindow)
	{
		//前の区間で当てた敵にもう一回当たるようにする//これしないと2ヒット目以降すり抜ける
		col.ClearHitIds();

		//ノックバックは最後の一撃だけ//途中で吹っ飛ばすと残りが当たらないので
		bool isLastHit = (activeWindow == static_cast<int>(node.attackColStartFrames.size()) - 1);
		if (isLastHit)
		{
			//CSVの値そのまま//単発の攻撃はここしか通らないので今まで通り
			col.SetKnockBack(Vector3(node.knockBackXZ, node.knockBackY, 0), node.isKirimomi);
		}
		else
		{
			//途中はその場で削るだけ
			col.SetKnockBack(Vector3(0, 0, 0), false);
		}
	}
	m_lastAttackWindow = activeWindow;//次のフレーム用に覚えとく

	//ON/OFFは呼ぶ側でやる//ラストヒット演出とかで消したいときがあるので
	return activeWindow != -1;
}

bool PlayerStateAttackBase::UpdateDropAttackWindow(AttackCol& col, const ComboNode& node)
{
	auto player = m_owner.lock();
	if (!player) return false;

	//今どの区間にいるか探す//ここはUpdateAttackWindowと一緒
	int activeWindow = -1;
	for (int i = 0; i < static_cast<int>(node.attackColStartFrames.size()); ++i)
	{
		if (player->m_anim.IsAnimFrameBetween(node.attackColStartFrames[i], node.attackColEndFrames[i]))
		{
			activeWindow = i;
			break;
		}
	}

	//新しい区間に入った1フレームだけ通る
	//ノックバックはm_dropAttackColを作るときに0にしてるのでここでは触らない
	//(敵をEnemyHitDropに入れるのはm_attackColの仕事//ここで入れるとHitDropに入り直して重力がリセットされる)
	if (activeWindow != -1 && activeWindow != m_lastAttackWindow)
	{
		//前の区間で当てた敵にもう一回当たるようにする
		col.ClearHitIds();
	}
	m_lastAttackWindow = activeWindow;//次のフレーム用に覚えとく

	return activeWindow != -1;
}

void PlayerStateAttackBase::InitVerticalMove(const ComboNode& node)
{
	auto player = m_owner.lock();
	if (!player) return;
	//上下差がある攻撃の時はここで初速を決めておく//実際に動き出すのはmoveStartFrameから(AttackMoveMent)
	if (node.moveSpeedY != 0)
	{
		//上下の速度を保存
		m_InitVel = player->m_targetVec * node.moveSpeedX + Vector3(0, node.moveSpeedY, 0);
		m_gravity = 0.0f;
		player->m_rb.m_vel = Vector3(0, 0, 0);
	}
}

void PlayerStateAttackBase::InpuctAttackSetUp()
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
		player->m_burstAttackCol->SetIsActive(true);//攻撃の当たり判定を有効にする
	}
}

void PlayerStateAttackBase::CreateAttackCol(const ComboNode& node)
{
	auto player = m_owner.lock();
	if (!player) return;

	float totalAnimFrame = player->m_anim.GetAnimTotalFrame(node.animName);
	//ここでColliderを生成する//あとhitstopとkAttackColOffset
	player->m_attackData = {
	.attackPower = node.attackPower,
	.brokenRate = node.brokenRate,
	.knockBackPower = Vector3(node.knockBackXZ, node.knockBackY,0),
	//.knockBackPower = Vector3(0.0f,node.knockBackY,0.0f),//吹き飛ばない攻撃にする
	.knockBackFrame = totalAnimFrame,
	.hitStopTime = kHitStopTime,
	.kAttackColOffset = kAttackColOffset,
	.isKirimomi = node.isKirimomi,
	.attackType = node.attackType
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
	m_attackCol->SetIsActive(false);//最初は当たり判定を無効にしておく
	m_lastAttackWindow = -1;//前の段の区間番号が残ってると1ヒット目のクリアが飛ばされるのでリセット

	//下降攻撃のときだけ、追加ヒット用の判定も作る//それ以外はnullptrのまま
	if (node.moveSpeedY < 0)
	{
		m_dropAttackCol = std::make_shared<AttackCol>(m_owner, player->m_attackData);
		m_dropAttackCol->ColInit({
			.pos = player->m_rb.m_pos,
			.offset = offset,
			.shape = std::make_unique<SphereShape>(kAttackColRadius),
			.tag = {Collider::Faction::Player, Collider::ColRole::Attack},
			.isActive = false,
			.isTrigger = true
			});//m_attackColと同じ位置・大きさ//最初は無効
		//追加のヒットはダメージだけ//敵をEnemyHitDropに入れるのはm_attackColの仕事
		m_dropAttackCol->SetKnockBack(Vector3(0, 0, 0), false);
	}
}

void PlayerStateAttackBase::ReleaseAttackCol()
{
	//攻撃の当たり判定を削除する//
	if (m_attackCol)
	{
		CollisionManager::GetInstance().ReleaseCollider(m_attackCol);//当たり判定を削除する
		m_attackCol->SetIsActive(false);
		m_attackCol->SetLifeTimeLimited();
		m_attackCol.reset();
	}
	//下降攻撃の追加ヒット用も同じ消し方
	if (m_dropAttackCol)
	{
		CollisionManager::GetInstance().ReleaseCollider(m_dropAttackCol);
		m_dropAttackCol->SetIsActive(false);
		m_dropAttackCol->SetLifeTimeLimited();
		m_dropAttackCol.reset();
	}
}

float PlayerStateAttackBase::ResolveTransitionFrame(float csvFrame, float defaultRate)
{
	auto player = m_owner.lock();
	if (!player) return 0.0f;
	//CSVで指定されていたらそのフレームを使う
	if (csvFrame >= 0.0f) return csvFrame;
	//負の値なら、デフォルトの進行率をフレームに変換する
	return player->m_anim.GetAnimEndFrame() * defaultRate;
}
