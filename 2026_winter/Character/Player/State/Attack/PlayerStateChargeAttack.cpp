#include "PlayerStateChargeAttack.h"
#include "Character/Player/Player.h"
#include "Main/Input.h"
#include "Main/System.h"
#include "Render/EffectManager.h"
#include "Collision/AttackCol.h"
#include "Collision/SphereShape.h"

namespace
{
	constexpr float kSlashEffectScale = 20.0f;//斬撃のエフェクト//1/20で作っている

	//判定の形//タイミング・攻撃力・吹き飛ばしはComboChain01.csv
	constexpr float kAttackColRadius = 400.0f;//攻撃判定の半径

	constexpr float kPlayerCenter = 100.0f;//プレイヤーの当たり判定の中心点までのy軸の距離
}

PlayerStateChargeAttack::PlayerStateChargeAttack(std::weak_ptr<Player> player) :
	PlayerStateAttackBase(player)
{

}

PlayerStateChargeAttack::~PlayerStateChargeAttack()
{
}

void PlayerStateChargeAttack::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	player->SetPreventFall(true);
	DecideAttackDirection();

	StartPhase(Phase::First);
}

void PlayerStateChargeAttack::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	auto& input = Input::GetInstance();

	AttackMoveMent();

	//1段目が終わったら2段目へ
	if (m_phase == Phase::First && player->m_anim.GetAnimEndFlag())
	{
		StartPhase(Phase::Second);
	}

	//TODO: 2段目が終わったらIdle/Move
	if (m_phase == Phase::Second && player->m_anim.GetAnimEndFlag())
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

	

	player->m_anim.Update();
}

void PlayerStateChargeAttack::Exit()
{
	auto player = m_owner.lock();
	if (player)
	{
		player->SetPreventFall(false);
		player->m_comboInfo.currentComboIndex = ComboIndex::None;
		player->m_comboInfo.isHit = false;
	}
	ReleaseAttackCol();
}

void PlayerStateChargeAttack::DebugDraw()
{
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:ChargeAttack %s",
		m_phase == Phase::First ? "First" : "Second");
}

void PlayerStateChargeAttack::StartPhase(Phase phase)
{
	auto player = m_owner.lock();
	if (!player) return;

	ReleaseAttackCol();//前の段の判定
	m_phase = phase;


	const int comboIndex = (phase == Phase::First) ? ComboIndex::ChargeAttack1 : ComboIndex::ChargeAttack2;
	player->m_comboInfo.currentComboIndex = comboIndex;
	const ComboNode& node = player->GetComboNode(comboIndex);
	player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, node.animName, false, node.animTimeScale, node.endFrame, 0.0f, node.blendFrame);

	//当たり判定の処理
	CreateAttackCol(node);

	m_isSwingSePlayed = false;

	//斬撃のエフェクト//+Zを前にして作っているので、ゲームでは180度回す
	const Vector3 forward = player->GetTargetVec();
	const AsyncData effect = (phase == Phase::First) ? AsyncData::ChargeSlash1Effect : AsyncData::ChargeSlash2Effect;
	EffectManager::GetInstance().Play(effect, player->m_rb.GetPos(), atan2f(forward.x, forward.z) + DX_PI_F, kSlashEffectScale);

}

void PlayerStateChargeAttack::CreateAttackCol(const ComboNode& node)
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
	.hitStopTime = node.hitStopTime,
	.hitStopFrame = node.hitStopFrame,
	.ownHitStop = node.ownHitStop,
	.ownHitStopFrame = node.ownHitStopFrame,
	.allTimeScaleFlag = node.allTimeScaleFlag,
	.kAttackColOffset = 0.0f,
	.isKirimomi = node.isKirimomi,
	.attackType = node.attackType,
	.comboIndex = player->m_comboInfo.currentComboIndex,
	.isCameraShake = node.isCameraShake,
	.shakePower = node.shakePower
	};

	m_attackCol = std::make_shared<AttackCol>(m_owner, player->m_attackData);
	//Vector3 offset = player->m_targetVec * player->m_attackData.kAttackColOffset
	//	+ Vector3(0, kPlayerCenter, 0);//プレイヤーの前方に50.0f、y軸方向にkPlayerCenterだけオフセットする
	Vector3 offset = Vector3(0.0f, kPlayerCenter,0.0f);

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

	m_attackCol->SetShape(std::make_unique<SphereShape>(kAttackColRadius));
}

void PlayerStateChargeAttack::AttackMoveMent()
{
	//TODO: チャージ攻撃用の移動
	PlayerStateAttackBase::AttackMoveMent();
}
