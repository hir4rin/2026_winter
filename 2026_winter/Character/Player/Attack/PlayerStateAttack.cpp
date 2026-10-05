#include "PlayerStateAttack.h"
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
#include "../../../Camera/LockOnManager.h"
#include "../../Enemy/EnemyBase.h"
#include "../../Enemy/EnemyManager.h"
//#include "../Effect/EffectManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	//CSVの遷移フレームが空欄(-1)のときに使うデフォルトの進行率
	constexpr float kDefaultComboInputStartRate = 0.2f;//コンボ入力受付開始
	constexpr float kDefaultComboInputEndRate = 0.8f;//コンボ入力受付終了
	constexpr float kDefaultCancelRate = 0.5f;//予約済みの次のコンボに移行する
	constexpr float kDefaultActionCancelRate = 0.5f;//回避・ジャンプ・確殺でキャンセルできる

	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間
	constexpr float kAttackColOffset = 30.0f;//攻撃の当たり判定を前に出す距離
	constexpr float kAttackColRadius = 150.0f;//攻撃の当たり判定の半径

	const Vector3 kDropAttackKnockBack = Vector3(20.0f, 20.0f, 0.0f);//ドロップ攻撃着地時のノックバック量
	const Vector3 kDropAttackColOffset = Vector3(0.0f, 50.0f, 0.0f);//ドロップ攻撃着地時の当たり判定オフセット
	constexpr float kDropAttackColLifeTime = 10.0f;//ドロップ攻撃着地時の当たり判定の生存時間

	constexpr float kEnemyTargetConeAngle = DX_PI_F / 3.0f;//入力方向にいる敵をターゲットにする角度範囲(60度)
	constexpr float kNearbyEnemyRangeMultiplier = 2.0f;//近くの敵を集める範囲(ロックオン範囲の倍率)

	//アニメーションの上昇(ルートモーション)を見た目から消す攻撃//ComboChain.csvのindex(切り上げ攻撃)
	//ComboIndex::upAttackはCSVとずれているので、CSVの値を直接使う
	constexpr int kRootMotionCancelComboIndexUp = 8;
	//アニメーションの下降を見た目から消すときは、SetRootMotionEnable(RootMotionCancel::Down)を使う
	constexpr int kRootMotionCancelComboIndexDown = 13;

	//着地後にPlayerStateAttackLanding(着地硬直)へ遷移する攻撃//ComboChain.csvのindex
	constexpr int kLightAttackLandingComboIndex = 13;//空中弱攻撃5(最終段)
	constexpr int kHeavyAttackLandingComboIndex = 14;//空中強攻撃1
}


PlayerStateAttack::PlayerStateAttack(std::weak_ptr<Player> player, AttackType type) :
	PlayerStateAttackBase(player), m_attackType(type)
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
	//攻撃の方向を決める(入力→ロックオン→内部ターゲット)
	DecideAttackDirection();

	//アニメーションの初期化//コンボの段数によってアニメーションを変える//-1はplayerがいないとき
	int currentComboIndex = SelectAnimInit();
	const ComboNode& node = player->m_comboChain[currentComboIndex];
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame);
	//切り上げ攻撃は、上昇をプログラム(moveSpeedY)で行うので、アニメーションの上昇を見た目から消す
	//アニメーションが切り替わってブレンドが終わると、Animation側で自動的に解除される
	if (node.index == kRootMotionCancelComboIndexUp)
	{
		player->m_anim.SetRootMotionEnable(RootMotionCancel::Up);
	}
	//下に落ちる攻撃は、下降をプログラム(moveSpeedY)で行うので、アニメーションの下降を見た目から消す
	if(node.index == kRootMotionCancelComboIndexDown)
	{
		player->m_anim.SetRootMotionEnable(RootMotionCancel::Down);
	}

	//切り上げ攻撃の時は、足元にエフェクトを出す(座標更新は不要、出すだけでいい)
	if (node.index == ComboIndex::upAttack)
	{
		//EffectManager::GetInstance().Play(AsyncData::JumpAttackFootEffect, player->m_rb.m_pos);
	}

	//上下差がある攻撃の時はここで初速を決めておく//実際に動き出すのはmoveStartFrameから(AttackMoveMent)
	InitVerticalMove(node);
	//攻撃データと当たり判定を生成する//最初は無効
	CreateAttackCol(node);

	m_isSwingSePlayed = false;//振りのSEをまだ再生していない状態にする
}

void PlayerStateAttack::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//攻撃中の移動処理
	AttackMoveMent();
	//振りのSEを出す
	SwingSeCheck();


	//コンボ予約の入力を取る//予約を取ったらもうここは通らないようにする
	AttackInputCheck();

	const ComboNode& currentNode = player->m_comboChain[player->m_comboInfo.currentComboIndex];
	float nowFrame = player->m_anim.GetNowAnimFrame();
	float actionCancelFrame = ResolveTransitionFrame(currentNode.actionCancelFrame, kDefaultActionCancelRate);
	float cancelFrame = ResolveTransitionFrame(currentNode.cancelFrame, kDefaultCancelRate);

	//actionCancelFrameを過ぎたら、回避・ジャンプ・確殺でキャンセルできる
	if (nowFrame >= actionCancelFrame && player->IsFloor())
	{
		//確殺
		if (input.IsTriggered("Y") && player->CanPartBrokenFinish())
		{
			player->ChangeState(std::make_shared<PlayerStatePartBrokenKill>(m_owner));
			return;
		}
		//回避
		if (input.IsTriggered("B"))
		{
			player->ChangeState(std::make_shared<PlayerStateDodge>(m_owner));
			return;
		}
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
	}
	
	//cancelFrameを過ぎたら、予約済みのコンボに移行
	if (nowFrame >= cancelFrame)
	{
		//通常攻撃からスキル攻撃に移行するとき//コンボではなく、スキル攻撃を初めて降ったというシステム
		if (m_isSkillAttackReserved)
		{
			m_isSkillAttackReserved = false;//スキル攻撃の予約を解除する
			AttackFinishProcess();//攻撃の段数を初期化するなどの処理//ここでcurrentComboIndexがNoneに戻るので、スキル攻撃1から始まる
			//スキルゲージを減らす
			player->AddSkillGauge(-player->kSkillAttackGaugeCost);

			//スキル攻撃に移行する
			player->ChangeState(std::make_shared<PlayerStateSkillAttack>(m_owner));
			return;
		}

		//コンボ入力が予約されているとき
		if (m_isComboInputReserved)
		{
			//次のコンボに移行する
			StartCombo(m_nextComboIndex);//m_nextComboIndexもm_currentComboIndexも更新されている
			m_isComboTransition = true;//Exitでコンボ段数をリセットしないようにする
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
			//空中弱攻撃の最終段・空中強攻撃は着地硬直へ
			if (currentComboIndex == kLightAttackLandingComboIndex)
			{
				player->ChangeState(std::make_shared<PlayerStateAttackLanding>(m_owner, AttackType::lightAttack));
				return;
			}
			if (currentComboIndex == kHeavyAttackLandingComboIndex)
			{
				player->ChangeState(std::make_shared<PlayerStateAttackLanding>(m_owner, AttackType::heavyAttack));
				return;
			}
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
			return;
		}
	}

	//アニメーションの更新
	player->m_anim.Update();
}

void PlayerStateAttack::Exit()
{
	//次のコンボ段への遷移以外(回避・確殺・被弾・落下など)で抜けたときは、コンボ段数をリセットする
	//リセットしないと、次の攻撃でSelectAnimInitが前の段の続きとして再生してしまう
	auto player = m_owner.lock();
	if (player && !m_isComboTransition)
	{
		player->m_comboInfo.currentComboIndex = ComboIndex::None;//攻撃していない状態に戻す
		player->m_comboInfo.isHit = false;
		player->m_isRaven = false;//鴉状態を解除する
	}

	//攻撃の当たり判定を削除する
	ReleaseAttackCol();
}

void PlayerStateAttack::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Attack");
	DrawFormatString(10, 30, GetColor(255, 255, 255), "ComboIndex:%d", m_owner.lock()->m_comboInfo.currentComboIndex);
	DrawFormatString(10, 50, GetColor(255, 255, 255), "m_attackCol Active:%d", m_attackCol->GetIsActive());
	//isHitの表示
	DrawFormatString(10, 70, GetColor(255, 255, 255), "isHit:%d", m_owner.lock()->m_comboInfo.isHit);
}

void PlayerStateAttack::CheckNoLockOnTargetEnemyForAirAttack5()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//ロックオンしていたらreturnする
	if (player->IsLockOn())return;

	//スティックの入力方向を求める
	Vector3 inputDir = Vector3(0, 0, 0);
	if (input.IsPressed("Up")) inputDir += player->forward;
	if (input.IsPressed("Down")) inputDir += player->down;
	if (input.IsPressed("Left")) inputDir += player->left;
	if (input.IsPressed("Right")) inputDir += player->right;
	bool hasInput = inputDir.Magnitude() > 0.0f;

	//入力がないとき
	if (!hasInput)
	{
		//地上の内部ターゲットがいれば、そのまま使う
		auto softTarget = player->GetSoftTarget();
		if (softTarget && softTarget->IsFloor())return;

		//いなければ、カメラの向いている方向から探す
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

	//プレイヤーの一定範囲内にいる地上の敵を集める
	auto enemyManager = player->m_enemyManager.lock();
	if (!enemyManager)return;
	float range = player->GetCameraRockOnRange() * kNearbyEnemyRangeMultiplier;
	float cosTheta = cosf(kEnemyTargetConeAngle);//この角度以内の敵をターゲットにする//cosでの判定に使う

	//入力方向とのcosが最大の敵をターゲットにする
	std::shared_ptr<EnemyBase> bestTarget = nullptr;
	float maxCos = -1.0f;
	for (auto& enemy : enemyManager->GetEnemies())
	{
		if (!enemy)continue;
		if (enemy->GetIsLifeZero())continue;
		//地上の敵だけを対象にする
		if (!enemy->IsFloor())continue;

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
		//見つかったら内部ターゲットにする
		player->SetSoftTarget(bestTarget);
	}
	else
	{
		//地上の敵がいないときは、前の内部ターゲット(空中の敵など)を狙わない
		player->ClearSoftTarget();
	}
}

bool PlayerStateAttack::SearchNearbyAirEnemy()
{
	auto player = m_owner.lock();
	if (!player) return false;
	auto enemyManager = player->m_enemyManager.lock();
	if (!enemyManager)return false;

	//CheckNoLockOnTargetEnemyと同じ範囲で探す
	float range = player->GetCameraRockOnRange() * kNearbyEnemyRangeMultiplier;
	for (auto& enemy : enemyManager->GetEnemies())
	{
		if (!enemy)continue;
		if (enemy->GetIsLifeZero())continue;
		if (enemy->IsFloor())continue;//地上の敵は対象外

		Vector3 enemyPos = enemy->GetRigidBody().GetPos();
		if ((enemyPos - player->m_rb.m_pos).Magnitude() >= range)continue;

		//空中の敵が1体でも見つかったら終了
		return true;
	}
	return false;
}

void PlayerStateAttack::AttackInputCheck()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();
	//既に予約があるならリターン
	if (m_isComboInputReserved) return;

	//現在のコンボノードを取得
	int currentComboIndex = player->m_comboInfo.currentComboIndex;
	const ComboNode& currentNode = player->m_comboChain[currentComboIndex];

	//攻撃の入力を受け付けるか
	float inputStartFrame = ResolveTransitionFrame(currentNode.comboInputStartFrame, kDefaultComboInputStartRate);
	float inputEndFrame = ResolveTransitionFrame(currentNode.comboInputEndFrame, kDefaultComboInputEndRate);
	float nowFrame = player->m_anim.GetNowAnimFrame();
	bool canInput = nowFrame >= inputStartFrame && nowFrame <= inputEndFrame;//コンボ入力受付時間内かどうか
	if (!canInput)return;//コンボ入力受付時間外なら、ここで処理を終える
	bool isPlayerAir = !player->IsFloor();//空中にいるかどうか
	bool WasSkillAirAttack = player->m_comboInfo.isAirSkillAttack;//空中でスキル攻撃をしたかどうか

	//スキル攻撃//コンボを終了して、PlayerStateSkillAttackに移行する
	if (input.IsTriggered("LB"))
	{
		//スキル攻撃ができるかどうか//ゲージは移行するときに減らす
		if (player->CanSkillAttack(false))
		{
			//空中でスキル攻撃を行っていたらスキル攻撃に移行しない
			if (isPlayerAir && WasSkillAirAttack)return;

			m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
			m_isSkillAttackReserved = true;//スキル攻撃の予約がされているフラグを立てる
		}
	}
	//強攻撃
	else if (!input.IsPressed("LB") && input.IsTriggered("Y"))
	{
		if (!currentNode.nextHeavyAttack.empty())//空じゃなかったら
		{
			m_nextComboIndex = currentNode.nextHeavyAttack[0];//次のコンボ番号をセットする//今回は1つしかないので、0番目をセットする
			m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
		}
	}
	//弱攻撃
	else if (!input.IsPressed("LB") && input.IsTriggered("X"))
	{
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
	m_attackCol->SetIsActive(false);
	m_attackCol->SetLifeTimeLimited();
	m_attackCol.reset();
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
		//スキル攻撃はPlayerStateSkillAttackで行うので、通常攻撃では鴉状態を解除する
		player->m_isRaven = false;//鴉状態を解除する
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
		player->m_isRaven = false;//鴉状態を解除する

		//コンボの段数が-1でないときは、次のコンボを再生する
		//currentComboIndex = m_nextComboIndex;//次のコンボの段数を取得する
		//player->m_comboInfo.currentComboIndex = currentComboIndex;//現在のコンボの段数を更新する
	}

	//攻撃が空中攻撃1の時、近くの敵をサーチして、空中にいる敵がいなかったら空中攻撃5に変更する
	if (currentComboIndex == ComboIndex::AirAttack1)
	{
		if (!SearchNearbyAirEnemy())
		{
			player->m_comboInfo.isAirAttack = true;
			currentComboIndex = ComboIndex::AirAttack5;
			//現在のコンボ番号も更新
			player->m_comboInfo.currentComboIndex = currentComboIndex;
			//ロックオンしていないときは、地上の敵を内部ターゲットにして、そっちに向き直す
			CheckNoLockOnTargetEnemyForAirAttack5();//本当にこの関数が必要だったかは審議
			NoLockOnAttackDirection();
		}
		//ロックオンしている敵が地上にいるなら、空中攻撃5にする
		if (player->IsLockOn())
		{
			bool enemyIsFloor = player->m_lockOnManager->GetLockTarget()->IsFloor();
			if (enemyIsFloor)
			{
				player->m_comboInfo.isAirAttack = true;
				currentComboIndex = ComboIndex::AirAttack5;
				//現在のコンボ番号も更新
				player->m_comboInfo.currentComboIndex = currentComboIndex;
			}
		}
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
		player->m_burstAttackCol->SetIsActive(true);//攻撃の当たり判定を有効にする
	}
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
