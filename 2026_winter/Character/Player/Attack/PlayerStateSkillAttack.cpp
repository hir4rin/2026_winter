#include "PlayerStateSkillAttack.h"
#include "Player.h"
#include "../../../Game.h"
#include "../../../Input.h"
#include "../../AttackCol.h"
#include "../../../Collider/SphereShape.h"

namespace
{
	constexpr float kComboInputStart = 0.2f;//コンボ入力受付開始のアニメーションの進行率
	constexpr float kComboInputEnd = 0.8f;//コンボ入力受付終了のアニメーションの進行率

	constexpr float kPlayerCenter = 100.0f;//プレイヤーの当たり判定の中心点までのy軸の距離

	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間
	constexpr float kAttackColOffset = 30.0f;//攻撃の当たり判定を前に出す距離
	constexpr float kAttackColRadius = 150.0f;//攻撃の当たり判定の半径
}

PlayerStateSkillAttack::PlayerStateSkillAttack(std::weak_ptr<Player> player):
	PlayerState(player)
{
	//playerが既に破棄されていたら早期リターンする//trueで破棄されている
	if (m_owner.expired())return;
	m_attackType = AttackType::SkillAttack;//スキル攻撃に設定
}

PlayerStateSkillAttack::~PlayerStateSkillAttack()
{
}

void PlayerStateSkillAttack::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//攻撃の方向を決める
	DetermineAttackDirection();
	//アニメーションの初期化//コンボの段数によってアニメーションを変える//-1はplayerがいないとき
	int currentComboIndex = SelectAnimInit();
	const ComboNode& node = player->m_comboChain[currentComboIndex];
	player->m_anim.ChangeAnim(node.animName, false, node.animTimeScale);
	//上下差がある攻撃の時はここで初速を与える
	if (node.moveSpeedY != 0)
	{
		player->m_rb.m_vel = player->m_targetVec * node.moveSpeedX + Vector3(0, node.moveSpeedY, 0);
		if (node.moveSpeedY > 0)
		{
			player->m_isGround = false;//ジャンプ状態にする
		}
	}
	float totalAnimFrame = player->m_anim.GetAnimTotalFrame(node.animName);
	//ここでColliderを生成する//あとhitstopとkAttackColOffset
	player->m_attackData = {
	.attackPower = node.attackPower,
	.brokenRate = node.brokenRate,
	//.knockBackPower = Vector3(node.knockBackXZ, node.knockBackY,0),
	.knockBackPower = Vector3(0.0f,node.knockBackY,0.0f),//吹き飛ばない攻撃にする
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
		.isActive = true,
		.isTrigger = true
		});//攻撃の当たり判定を初期化する//最初は無効にしておく
	m_attackCol->SetIsActive(false);//最初は当たり判定を無効にしておく

}

void PlayerStateSkillAttack::Update()
{
}

void PlayerStateSkillAttack::Exit()
{
}

void PlayerStateSkillAttack::DebugDraw()
{
}

void PlayerStateSkillAttack::AttackMoveMent()
{
}

void PlayerStateSkillAttack::DetermineAttackDirection()
{
	auto player = m_owner.lock();
	if (!player) return;

	auto& input = Input::GetInstance();
	Vector3 attackDir = Vector3(0, 0, 0);

	//攻撃の方向を決める//カメラの向きと入力から、回避の方向を決める
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

void PlayerStateSkillAttack::AttackInputCheck()
{
}

void PlayerStateSkillAttack::StartCombo(int comboIndex)
{
}

void PlayerStateSkillAttack::AttackFinishProcess()
{
}

int PlayerStateSkillAttack::SelectAnimInit()
{
	auto player = m_owner.lock();
	if (!player)return -1;

	//アニメーションの初期化
	int currentComboIndex = player->m_comboInfo.currentComboIndex;//現在のコンボインデックスを取得する
	if (currentComboIndex == ComboIndex::None)//コンボの段数が-1のときは、最初のコンボを再生する
	{
		if (m_attackType == AttackType::lightAttack)
		{
			if (player->IsFloor())currentComboIndex = ComboIndex::LightAttack1;//弱攻撃の最初の段数を0に設定する
		}
		else if (m_attackType == AttackType::heavyAttack)
		{
			if (player->IsFloor())currentComboIndex = ComboIndex::HeavyAttack1;//強攻撃の最初の段数を1に設定する//今回は、弱攻撃が0番目、強攻撃が1番目の段数から始まるようにする
		}
		if (m_attackType == AttackType::lightAttack)
		{
			if (player->IsFloor())currentComboIndex = ComboIndex::LightAttack1;//弱攻撃の最初の段数を0に設定する
			else currentComboIndex = ComboIndex::AirAttack1;//空中攻撃1
		}
		else if (m_attackType == AttackType::heavyAttack)
		{
			if (player->IsFloor())currentComboIndex = ComboIndex::HeavyAttack1;//強攻撃の最初の段数を1に設定する//今回は、弱攻撃が0番目、強攻撃が1番目の段数から始まるようにする
			else currentComboIndex = ComboIndex::AirHeavyAttack1;//空中強攻撃1
		}
		//現在のコンボの段数を更新する
		player->m_comboInfo.currentComboIndex = currentComboIndex;
	}
	else
	{
		//コンボの段数が-1でないときは、次のコンボを再生する
		//currentComboIndex = m_nextComboIndex;//次のコンボの段数を取得する
		//player->m_comboInfo.currentComboIndex = currentComboIndex;//現在のコンボの段数を更新する
	}
	return currentComboIndex;
}
