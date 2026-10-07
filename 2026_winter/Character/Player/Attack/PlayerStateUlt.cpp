#include "PlayerStateUlt.h"
#include "Player.h"
#include "../PlayerWeapon.h"
#include "../../../Input.h"
#include "../../AttackCol.h"
#include "../../../Collider/SphereShape.h"
#include "../../../Collider/BoxShape.h"
#include "../../../System.h"
#include "../../../BattleManager.h"
#include "EffekseerForDXLib.h"

namespace
{
	constexpr float kPlayerCenter = 100.0f;//プレイヤーの当たり判定の中心点までのy軸の距離

	//攻撃データ//後ほどデータ化する
	constexpr float kAttackDamage = 500.0f;//攻撃力
	constexpr float kBrokenRate = 100.0f;//部位破壊率(%)
	constexpr float kHitStopTime = 0.1f;//攻撃ヒット時のヒットストップ時間
	constexpr float kAttackColOffset = 400.0f;//攻撃判定を前に出す距離
	constexpr float kAttackColRadius = 500.0f;//攻撃判定の半径//周りの敵をまとめて巻き込む

	//エフェクトを出すアニメーションの進行率
	constexpr float kEffectTriggerRate = 0.5f;


	constexpr float stopAnimFrame = 1.58f;//Attackのアニメの手を上に上げた瞬間//一旦使っていない

	constexpr float kColStart = 20.0f;
	constexpr float kColEnd = 30.0f;

	const std::string kUltStartAnim = "Power_Attack_Start";//構え始め
	const std::string kUltLoopAnim = "Power_Attack_Loop";//構えたまま溜める
	constexpr int kUltLoopFrame = 60;//Loopを流し続けるフレーム数
}

PlayerStateUlt::PlayerStateUlt(std::weak_ptr<Player> player):
	PlayerStateAttackBase(player)
{
}

PlayerStateUlt::~PlayerStateUlt()
{
}

void PlayerStateUlt::Enter()
{
	auto player = m_owner.lock();
	if (!player) return;
	//攻撃の方向を決める(入力→ロックオン→内部ターゲット)
	DecideAttackDirection();

	//必殺技の間だけ刀身の長い刀に持ち替える
	player->m_weapon->SetUltWeapon(true);
	//System::GetInstance().GetSoundManager().PlaySE("UltAttackSE");

	//攻撃の当たり判定を生成する//最初は無効
	CreateUltAttackCol();

	//構え始めのアニメーションから流す
	ChangePhase(Phase::Start);
}

void PlayerStateUlt::ChangePhase(Phase phase)
{
	auto player = m_owner.lock();
	if (!player) return;

	m_phase = phase;
	switch (m_phase)
	{
	case Phase::Start:
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, kUltStartAnim, false,0.5f,9.0f);
		break;
	case Phase::Loop:
		m_loopFrame = 0;
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, kUltLoopAnim, true,0.5f);
		break;
	case Phase::Attack:
		player->m_anim.ChangeAnimWithModelHandle(player->m_modelHandle, player->GetAnimName("UltAttack"), false, 0.5f,50.0f);
		//必殺技本体からスローにする
		System::GetInstance().SetTimeScale(0.2f);
		break;
	}
}

void PlayerStateUlt::Update()
{
	auto player = m_owner.lock();
	if (!player) return;

	switch (m_phase)
	{
	case Phase::Start:
		//構え始めが終わったら溜めへ
		if (player->m_anim.GetAnimEndFlag())
		{
			ChangePhase(Phase::Loop);
			return;
		}
		player->m_anim.Update();
		break;
	case Phase::Loop:
		//一定時間溜めたら必殺技本体へ
		if (++m_loopFrame >= kUltLoopFrame)
		{
			ChangePhase(Phase::Attack);
			return;
		}
		player->m_anim.Update();
		break;
	case Phase::Attack:
		UpdateAttack();
		break;
	}
}

void PlayerStateUlt::UpdateAttack()
{
	auto player = m_owner.lock();
	if (!player) return;
	auto& input = Input::GetInstance();

	//当たり判定のON/OFF
	UpdateUltAttackCol();
	//エフェクトを出す//出ていたら追従させる
	EffectCheck();


	//このフレームに来たら一旦アニメーションを止めて遷移させる
	if (player->m_anim.GetNowAnimFrame() >= 1.58f && !m_isTriggeredStopAnim && m_phase == Phase::Attack)
	{
		//System::GetInstance().SetTimeScaleForFrames(0.001f,150.0f);
		System::GetInstance().SetTimeScale(1.0f);
		m_isTriggeredStopAnim = true;
	}


	//単発なのでアニメーションが終わるまでキャンセルできない
	if (player->m_anim.GetAnimEndFlag())
	{
		if (player->IsFloor())
		{
			if (input.IsLeftStickInput())
			{
				player->ChangeState(std::make_shared<PlayerStateMove>(m_owner));
				return;
			}
			player->ChangeState(std::make_shared<PlayerStateIdle>(m_owner));
			return;
		}
		//空中は落下状態に移行
		player->ChangeState(std::make_shared<PlayerStateFall>(m_owner));
		return;
	}

	//アニメーションの更新
	player->m_anim.Update();
}

void PlayerStateUlt::Exit()
{
	auto player = m_owner.lock();
	if (!player) return;

	//専用必殺技を出し終わったら必殺技状態は終わり
	player->m_isUltimating = false;
	//通常の刀に戻す
	player->m_weapon->SetUltWeapon(false);

	//攻撃の当たり判定を削除する
	ReleaseAttackCol();
	//エフェクトは止めずに最後まで流す
}

void PlayerStateUlt::DebugDraw()
{
	auto player = m_owner.lock();
	if (!player) return;
	DrawFormatString(10, 10, GetColor(255, 255, 255), "PlayerState:Ult");
	DrawFormatString(10, 30, GetColor(255, 255, 255), "AnimRate:%.2f", player->m_anim.GetAnimRate());
	if (m_attackCol)DrawFormatString(10, 50, GetColor(255, 255, 255), "m_attackCol Active:%d", m_attackCol->GetIsActive());
}

void PlayerStateUlt::CreateUltAttackCol()
{
	auto player = m_owner.lock();
	if (!player) return;

	//吹き飛ばさない攻撃//演出(スロー・カメラ)はAttackColのUltAttackの処理で行う
	player->m_attackData = {
	.attackPower = kAttackDamage,
	.brokenRate = kBrokenRate,
	.knockBackPower = Vector3(0, 0, 0),
	.knockBackFrame = 0,
	.hitStopTime = kHitStopTime,
	.kAttackColOffset = kAttackColOffset,
	.isKirimomi = false
	};

	m_attackCol = std::make_shared<AttackCol>(m_owner, player->m_attackData);
	Vector3 offset = player->m_targetVec * player->m_attackData.kAttackColOffset
		+ Vector3(0, kPlayerCenter, 0);//プレイヤーの前方とy軸方向にkPlayerCenterだけオフセットする
	//回転の角度//プレイヤーの向いている方向にBOXのローカルの+Zを合わせる
	float rotY = atan2f(player->m_targetVec.x, player->m_targetVec.z);

	m_attackCol->ColInit({
		.pos = player->m_rb.m_pos,
		.offset = offset,
		.shape = std::make_unique<BoxShape>(Vector3(kAttackColRadius,100.0f,kAttackColRadius),rotY),
		.tag = {Collider::Faction::Player, Collider::ColRole::UltAttack},
		.isActive = false,
		.isTrigger = true
		});
}

void PlayerStateUlt::UpdateUltAttackCol()
{
	if (!m_attackCol) return;
	auto player = m_owner.lock();
	if (!player) return;

	//ラストヒットの演出中は判定を出さない
	if (System::GetInstance().GetBattleMgr()->GetIsLastHitEventPlaying())
	{
		m_attackCol->SetIsActive(false);
		return;
	}

	if (player->m_anim.IsAnimFrameBetween(kColStart, kColEnd))
	{
		m_attackCol->SetIsActive(true);
	}
	else
	{
		m_attackCol->SetIsActive(false);
	}
}

void PlayerStateUlt::EffectCheck()
{
	auto player = m_owner.lock();
	if (!player)return;

	//まだ出していなくて、出すタイミングになったら出す
	if (!m_isTriggeredEffect)
	{
		if (player->m_anim.GetAnimRate() < kEffectTriggerRate)return;
		player->m_efPlayingHandle = PlayEffekseer3DEffect(player->m_efHandle);
		m_isTriggeredEffect = true;
	}

	//座標と向きをプレイヤーに合わせる
	SetPosPlayingEffekseer3DEffect(player->m_efPlayingHandle, player->m_rb.m_pos.x, player->m_rb.m_pos.y + kPlayerCenter, player->m_rb.m_pos.z);
	SetRotationPlayingEffekseer3DEffect(player->m_efPlayingHandle, 0.0f, player->m_rotAngleY + DX_PI_F, 0.0f);
}
