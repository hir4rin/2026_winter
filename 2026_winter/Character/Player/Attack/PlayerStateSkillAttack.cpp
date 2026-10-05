#include "PlayerStateSkillAttack.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../AttackCol.h"
#include "../System.h"
#include "EffekseerForDXLib.h"
#include "../../Enemy/EnemyBase.h"

namespace
{
	//CSVの遷移フレームが空欄(-1)のときに使うデフォルトの進行率
	constexpr float kDefaultComboInputStartRate = 0.2f;//コンボ入力受付開始
	constexpr float kDefaultComboInputEndRate = 0.8f;//コンボ入力受付終了
	constexpr float kDefaultCancelRate = 0.5f;//予約済みの次のコンボに移行する
	constexpr float kDefaultActionCancelRate = 0.5f;//回避・ジャンプ・確殺でキャンセルできる

	constexpr float kEffectTriggerTime = 0.2f;//エフェクトを出すタイミング
	constexpr float kEffect2TriggerTime = 0.4f;//エフェクトを出すタイミング
	const Vector3 kSkillEffectOffset = Vector3(0.0f, 100.0f, 0.0f);//スキル攻撃エフェクトのオフセット

	//スキル1,2の分身エフェクト(GhostDash/GhostDash3D)
	constexpr float kGhostDashScale = 30.0f;//分身モデルは1/20で作っているので20倍して元の大きさに戻す
	constexpr float kGhostDashForwardDistance = 0.0f;//プレイヤーの正面にずらす距離//SceneMainの確認用(F5/F6)では300

	//スキル攻撃のコンボ番号かどうか
	bool IsSkillComboIndex(int comboIndex)
	{
		return comboIndex == ComboIndex::SkillAttack0 ||
			comboIndex == ComboIndex::SkillAttack1 ||
			comboIndex == ComboIndex::SkillAttack2 ||
			comboIndex == ComboIndex::SkillAttack3;
	}

	//打ち上げ(スキル0)の次に自動で出す段//CSVのnextLightAttack//空欄ならスキル攻撃1
	int GetLaunchNextComboIndex(const ComboNode& launchNode)
	{
		return launchNode.nextWeakAttack.empty() ? ComboIndex::SkillAttack1 : launchNode.nextWeakAttack[0];
	}
}

PlayerStateSkillAttack::PlayerStateSkillAttack(std::weak_ptr<Player> player):
	PlayerStateAttackBase(player)
{
}

PlayerStateSkillAttack::~PlayerStateSkillAttack()
{
}

void PlayerStateSkillAttack::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//攻撃の方向を決める(入力→ロックオン→内部ターゲット)
	DecideAttackDirection();

	//スキル攻撃の何段目を再生するか決める
	int currentComboIndex = SelectAnimInit();
	const ComboNode& node = player->m_comboChain[currentComboIndex];
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame);
	//打ち上げは上昇をプログラム(moveSpeedY)で行うので、アニメーションの上昇を見た目から消す
	if (currentComboIndex == ComboIndex::SkillAttack0)
	{
		//player->m_anim.SetRootMotionEnable(RootMotionCancel::Up);
	}

	//スキル攻撃中は鴉状態にする
	player->m_isRaven = true;

	//スキル1,2はモデルを消して、代わりに分身エフェクトを出す
	PlayGhostEffect(currentComboIndex);

	//打ち上げから続くスキル1以外はカメラの注視点を通常に戻す
	InitLaunchCamera(currentComboIndex);

	//上下差がある攻撃の時はここで初速を決めておく//実際に動き出すのはmoveStartFrameから(AttackMoveMent)
	InitVerticalMove(node);
	//攻撃データと当たり判定を生成する//最初は無効
	CreateAttackCol(node);
}

void PlayerStateSkillAttack::Update()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//攻撃中の移動処理
	AttackMoveMent();
	//エフェクトを出す
	EffectCheck();
	//分身エフェクトをプレイヤーに追従させる
	UpdateGhostEffect();

	const ComboNode& currentNode = player->m_comboChain[player->m_comboInfo.currentComboIndex];
	float nowFrame = player->m_anim.GetNowAnimFrame();
	float actionCancelFrame = ResolveTransitionFrame(currentNode.actionCancelFrame, kDefaultActionCancelRate);
	float cancelFrame = ResolveTransitionFrame(currentNode.cancelFrame, kDefaultCancelRate);

	//打ち上げ(スキル0)は入力を受け付けず、cancelFrameを過ぎたら自動で次のスキル攻撃へ移行する
	if (currentNode.index == ComboIndex::SkillAttack0)
	{
		//打ち上げが当たったら、カメラの注視点のYを打ち上げた敵に合わせる
		UpdateLaunchCamera();
		if (nowFrame >= cancelFrame)
		{
			int nextIndex = GetLaunchNextComboIndex(currentNode);
			//ゲージは打ち上げに入るときに減らしているので、ここでは減らさない
			StartNextSkill(nextIndex);
			return;
		}
		//アニメーションの更新
		player->m_anim.Update();
		return;
	}

	//コンボ予約の入力を取る//予約を取ったらもうここは通らないようにする
	AttackInputCheck();

	//actionCancelFrameを過ぎたら、回避・ジャンプ・確殺でキャンセルできる
	//コンボ段数のリセットはExitで行う
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
			player->m_rb.m_vel.y = 0.0f;
			player->ChangeState(std::make_shared<PlayerStateJump>(m_owner));
			return;
		}
	}

	//cancelFrameを過ぎたら、予約済みの攻撃に移行
	if (nowFrame >= cancelFrame && m_isComboInputReserved)
	{
		m_isComboInputReserved = false;//コンボ入力の予約を解除する
		//スキル攻撃の次の段へ//同じクラスなのでコンボ段数を引き継ぐ
		if (m_nextAttackType == AttackType::SkillAttack)
		{
			//スキル攻撃2,3はここでスキルゲージを減らす
			player->AddSkillGauge(-player->kSkillAttackGaugeCost);
			StartNextSkill(m_nextComboIndex);
			return;
		}
		//弱・強攻撃へ//別クラスなのでExitでコンボ段数をNoneに戻し、PlayerStateAttack側で地上/空中の1段目を選ばせる
		player->ChangeState(std::make_shared<PlayerStateAttack>(m_owner, m_nextAttackType));
		return;
	}

	//コンボ予約がなく、アニメーションが終わったとき
	if (player->m_anim.GetAnimEndFlag())
	{
		player->m_rb.m_vel.y = 0.0f;//y軸の速度を0に戻す
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

	//アニメーションの更新
	player->m_anim.Update();
}

void PlayerStateSkillAttack::Exit()
{
	//スキル攻撃の次の段への遷移以外で抜けたときは、コンボ段数をリセットする
	//PlayerStateAttackへ移るときもここでNoneに戻すので、通常攻撃は1段目から始まる
	auto player = m_owner.lock();
	if (player && !m_isComboTransition)
	{
		player->m_comboInfo.currentComboIndex = ComboIndex::None;//攻撃していない状態に戻す
		player->m_comboInfo.isHit = false;
		player->m_isRaven = false;//鴉状態を解除する
		player->m_isCameraFocusOverride = false;//カメラの注視点を通常に戻す
	}

	//分身エフェクトを止めて、モデルを表示に戻す//次の段で必要ならEnterでまた消す
	StopGhostEffect();

	//攻撃の当たり判定を削除する
	ReleaseAttackCol();
}

void PlayerStateSkillAttack::DebugDraw()
{
	auto player = m_owner.lock();
	if (!player) return;
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:SkillAttack");
	DrawFormatString(10, 30, GetColor(255, 255, 255), "ComboIndex:%d", player->m_comboInfo.currentComboIndex);
	if (m_attackCol)DrawFormatString(10, 50, GetColor(255, 255, 255), "m_attackCol Active:%d", m_attackCol->GetIsActive());
	DrawFormatString(10, 70, GetColor(255, 255, 255), "isHit:%d", player->m_comboInfo.isHit);
}

void PlayerStateSkillAttack::AttackInputCheck()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();
	//既に予約があるならリターン
	if (m_isComboInputReserved) return;

	//現在のコンボノードを取得
	const ComboNode& currentNode = player->m_comboChain[player->m_comboInfo.currentComboIndex];

	//攻撃の入力を受け付けるか
	float inputStartFrame = ResolveTransitionFrame(currentNode.comboInputStartFrame, kDefaultComboInputStartRate);
	float inputEndFrame = ResolveTransitionFrame(currentNode.comboInputEndFrame, kDefaultComboInputEndRate);
	float nowFrame = player->m_anim.GetNowAnimFrame();
	bool canInput = nowFrame >= inputStartFrame && nowFrame <= inputEndFrame;//コンボ入力受付時間内かどうか
	if (!canInput)return;//コンボ入力受付時間外なら、ここで処理を終える

	//スキル攻撃の次の段
	if (input.IsTriggered("LB"))
	{
		//スキル攻撃ができるかどうか//ゲージは移行するときに減らす
		if (!player->CanSkillAttack(false))return;
		//弱攻撃ボタンでつながる次のコンボがあるか
		if (currentNode.nextWeakAttack.empty())return;
		m_nextComboIndex = currentNode.nextWeakAttack[0];//次のコンボ番号をセットする//今回は1つしかないので、0番目をセットする
		m_nextAttackType = AttackType::SkillAttack;
		m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
	}
	//強攻撃につなぐ//地上なら強攻撃1、空中なら空中強攻撃1(PlayerStateAttack側で選ぶ)
	else if (!input.IsPressed("LB") && input.IsTriggered("Y"))
	{
		m_nextAttackType = AttackType::heavyAttack;
		m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
	}
	//弱攻撃につなぐ//地上なら弱攻撃1、空中なら空中弱攻撃1(PlayerStateAttack側で選ぶ)
	else if (!input.IsPressed("LB") && input.IsTriggered("X"))
	{
		//空中で既に弱攻撃をしていたらつながない
		if (!player->IsFloor() && player->m_comboInfo.isAirAttack)return;
		m_nextAttackType = AttackType::lightAttack;
		m_isComboInputReserved = true;//コンボ入力が予約されているフラグを立てる
	}
}

int PlayerStateSkillAttack::SelectAnimInit()
{
	auto player = m_owner.lock();
	if (!player)return -1;

	int currentComboIndex = player->m_comboInfo.currentComboIndex;//現在のコンボインデックスを取得する
	//スキル攻撃の次の段から来たとき以外(None、または他のステートの段数が残っていたとき)は、最初から始める
	if (!IsSkillComboIndex(currentComboIndex))
	{
		//地上なら打ち上げ(スキル0)から、空中ならそのままスキル攻撃1から
		currentComboIndex = player->IsFloor() ? ComboIndex::SkillAttack0 : ComboIndex::SkillAttack1;
		//現在のコンボの段数を更新する
		player->m_comboInfo.currentComboIndex = currentComboIndex;
	}
	//空中でスキル攻撃をしたフラグを立てる(着地するまでもう一度スキルは出せない)
	//地上にいる間はPlayer::Updateで毎フレームfalseに戻るので、打ち上げ後のスキル攻撃1に入ったときに立つ
	if (!player->IsFloor())player->m_comboInfo.isAirSkillAttack = true;
	return currentComboIndex;
}

void PlayerStateSkillAttack::StartNextSkill(int nextComboIndex)
{
	auto player = m_owner.lock();
	if (!player) return;
	//範囲外だったら早期リターン
	if (nextComboIndex < 0 || nextComboIndex >= player->m_comboChain.size())return;
	player->m_comboInfo.currentComboIndex = nextComboIndex;//現在のコンボの段数を更新
	player->m_comboInfo.isHit = false;
	m_isComboTransition = true;//Exitでコンボ段数をリセットしないようにする
	player->ChangeState(std::make_shared<PlayerStateSkillAttack>(m_owner));
}

void PlayerStateSkillAttack::PlayGhostEffect(int comboIndex)
{
	auto player = m_owner.lock();
	if (!player) return;

	//スキル1はGhostDash、スキル2はGhostDash3D//それ以外は分身を出さない
	AsyncData effectKey;
	if (comboIndex == ComboIndex::SkillAttack1)effectKey = AsyncData::DebugGhostDashEffect;
	else if (comboIndex == ComboIndex::SkillAttack2)effectKey = AsyncData::DebugGhostDash3DEffect;
	else return;

	//分身エフェクトが出ている間はモデルを描画しない
	player->m_isSkillInvisible = true;
	m_ghostEffectHandle = PlayEffekseer3DEffect(System::GetInstance().GetHandle(effectKey));
	SetScalePlayingEffekseer3DEffect(m_ghostEffectHandle, kGhostDashScale, kGhostDashScale, kGhostDashScale);
	UpdateGhostEffect();//最初の座標と向きを合わせる
}

void PlayerStateSkillAttack::UpdateGhostEffect()
{
	if (m_ghostEffectHandle == -1)return;
	auto player = m_owner.lock();
	if (!player) return;

	const Vector3 forward = player->m_targetVec;
	const Vector3 pos = GetSkillEffectBasePos() + forward * kGhostDashForwardDistance;
	SetPosPlayingEffekseer3DEffect(m_ghostEffectHandle, pos.x, pos.y, pos.z);
	//プレイヤーと同じ向きにする
	SetRotationPlayingEffekseer3DEffect(m_ghostEffectHandle, 0.0f, atan2f(forward.x, forward.z), 0.0f);
}

Vector3 PlayerStateSkillAttack::GetSkillEffectBasePos()
{
	auto player = m_owner.lock();
	if (!player) return Vector3();

	//敵に合わせるのはスキル0,1だけ//それ以外はプレイヤーの位置
	int comboIndex = player->m_comboInfo.currentComboIndex;
	if (comboIndex != ComboIndex::SkillAttack0 && comboIndex != ComboIndex::SkillAttack1)return player->m_rb.m_pos;

	//打ち上げ(スキル0)で当てた敵//いなければ攻撃の対象(ロックオン対象 or 内部ターゲット)
	auto enemy = player->m_cameraFocusEnemy.lock();
	if (!enemy || enemy->GetIsLifeZero())enemy = player->GetAttackTarget();
	if (enemy && !enemy->GetIsLifeZero())return enemy->GetRigidBody().GetPos();
	//敵がいないときはプレイヤーの位置
	return player->m_rb.m_pos;
}

void PlayerStateSkillAttack::StopGhostEffect()
{
	auto player = m_owner.lock();
	if (player)player->m_isSkillInvisible = false;//モデルを表示に戻す

	if (m_ghostEffectHandle == -1)return;
	StopEffekseer3DEffect(m_ghostEffectHandle);
	m_ghostEffectHandle = -1;
}

void PlayerStateSkillAttack::InitLaunchCamera(int comboIndex)
{
	auto player = m_owner.lock();
	if (!player) return;

	//打ち上げから続くスキル1:スキル0が当たっていれば、敵に合わせた注視点をそのまま続ける
	const int launchNextIndex = GetLaunchNextComboIndex(player->m_comboChain[ComboIndex::SkillAttack0]);
	if (comboIndex == launchNextIndex && player->m_isCameraFocusOverride)return;
	//それ以外(スキル0の開始、空中から直接スキル1、スキル2以降)は通常の注視点//スキル0は当たったらUpdateLaunchCameraで上書きする
	player->m_isCameraFocusOverride = false;
	player->m_cameraFocusEnemy.reset();
}

void PlayerStateSkillAttack::UpdateLaunchCamera()
{
	auto player = m_owner.lock();
	if (!player) return;
	//既に上書き中、またはまだ当たっていないなら何もしない
	if (player->m_isCameraFocusOverride)return;
	if (!player->m_comboInfo.isHit)return;
	//isHitは攻撃の対象(ロックオン対象 or 内部ターゲット)に当たったときに立つので、その敵を注視する
	auto enemy = player->GetAttackTarget();
	if (!enemy)return;
	player->m_cameraFocusEnemy = enemy;
	player->m_isCameraFocusOverride = true;
}

void PlayerStateSkillAttack::EffectCheck()
{
	//PlayerStateAttackから移動(コメントアウトのまま)
	//auto player = m_owner.lock();
	//if (!player)return;
	//int currentComboIndex = player->m_comboInfo.currentComboIndex;
	//const ComboNode& node = player->m_comboChain[currentComboIndex];
	//float rate = player->m_anim.GetAnimRate();//アニメーションの進行率を取得
	//if (currentComboIndex == ComboIndex::SkillAttack1)
	//{
	//	if (rate >= kEffectTriggerTime)
	//	{
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
